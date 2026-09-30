# -*- coding: utf-8 -*-
import tkinter as tk
from tkinter import ttk, messagebox
import asyncio
import threading
import queue
import csv
import os
import time
import numpy as np
import matplotlib
matplotlib.use("TkAgg")
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from matplotlib.figure import Figure
from bleak import BleakScanner, BleakClient

# Cấu hình UUID của ESP32 BLE
SERVICE_UUID = "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
CHARACTERISTIC_UUID = "beb5483e-36e1-4688-b7f5-ea07361b26a8"
DEVICE_NAME = "ESP32_ECG_AI_Tester"

class BLE_ECG_GUI:
    def __init__(self, root):
        self.root = root
        self.root.title("🩺 REAL-TIME ECG AI DASHBOARD (AD8232)")
        self.root.geometry("1200x750")
        self.root.configure(bg="#121212")
        
        # Đặt phong cách giao diện tối
        self.setup_styles()
        
        # Hàng đợi an toàn giữa luồng BLE và luồng GUI
        self.data_queue = queue.Queue(maxsize=1000)
        
        # Các biến lưu trạng thái kết nối
        self.is_connected = False
        self.client_thread = None
        self.scan_thread = None
        self.loop = None
        self.client = None
        self.sample_index = 0
        
        # Thiết bị được chọn (Địa chỉ MAC/UUID)
        self.selected_mac = None
        self.discovered_devices = {}  # Map: "Tên [MAC]" -> MAC
        
        self.last_bpm = 0.0
        self.last_spo2 = 0.0
        self.last_ppg_bpm = 0.0
        self.last_class = "N"
        self.last_probs = [0, 0, 0, 0, 0]
        self.heart_pulse_state = False
        
        # Bộ đệm vẽ đồ thị cuộn (Lưu 500 mẫu = 4 giây)
        self.buffer_size = 500
        self.raw_data = np.zeros(self.buffer_size)
        self.filt_data = np.zeros(self.buffer_size)
        
        # Ghi file kết quả CSV tương thích có chứa dấu thời gian thực (Timestamp)
        import datetime
        timestamp = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
        script_dir = os.path.dirname(os.path.abspath(__file__))
        self.output_csv = os.path.join(script_dir, f"ble_received_ecg_{timestamp}.csv")
        self.init_csv()

        # Khởi tạo giao diện
        self.build_gui()
        
        # Bắt đầu vòng lặp cập nhật giao diện (16ms = ~60 FPS)
        self.update_gui_loop()
        
    def setup_styles(self):
        self.style = ttk.Style()
        self.style.theme_use("clam")
        
        # Cấu hình màu sắc thẻ tối
        self.style.configure(".", background="#121212", foreground="#ffffff")
        self.style.configure("Card.TFrame", background="#1e1e1e", relief="flat")
        self.style.configure("Title.TLabel", background="#1e1e1e", foreground="#4caf50", font=("Outfit", 12, "bold"))
        self.style.configure("Status.TLabel", background="#1e1e1e", foreground="#aaaaaa", font=("Inter", 10))
        self.style.configure("TButton", font=("Inter", 9, "bold"), background="#4caf50", foreground="#ffffff", borderwidth=0)
        self.style.map("TButton", background=[("active", "#45a049")])
        self.style.configure("Scan.TButton", font=("Inter", 9, "bold"), background="#2196f3", foreground="#ffffff", borderwidth=0)
        self.style.map("Scan.TButton", background=[("active", "#1976d2")])
        self.style.configure("Disconnect.TButton", font=("Inter", 9, "bold"), background="#f44336", foreground="#ffffff", borderwidth=0)
        self.style.map("Disconnect.TButton", background=[("active", "#da190b")])
        
    def init_csv(self):
        file_exists = os.path.exists(self.output_csv)
        self.csv_file = open(self.output_csv, mode="a", newline="", encoding="utf-8")
        self.csv_writer = csv.writer(self.csv_file)
        if not file_exists or os.path.getsize(self.output_csv) == 0:
            self.csv_writer.writerow([
                "Index", "Raw", "Filtered", "Peak", "BPM", 
                "Class_ID", "Class_Name", "P0", "P1", "P2", "P3", "P4"
            ])
            self.csv_file.flush()

    def build_gui(self):
        # Frame chính chia làm 2 cột: Cột trái (Đồ thị), Cột phải (Điều khiển & AI)
        main_frame = tk.Frame(self.root, bg="#121212")
        main_frame.pack(fill="both", expand=True, padx=15, pady=15)
        
        # --- CỘT TRÁI: ĐỒ THỊ ---
        left_frame = tk.Frame(main_frame, bg="#121212")
        left_frame.pack(side="left", fill="both", expand=True, padx=(0, 10))
        
        # Biểu đồ Matplotlib
        self.fig = Figure(figsize=(8, 6), dpi=100, facecolor="#121212")
        self.ax_raw = self.fig.add_subplot(211, facecolor="#1a1a1a")
        self.ax_filt = self.fig.add_subplot(212, facecolor="#1a1a1a")
        
        # Cấu hình nhãn biểu đồ thô
        self.ax_raw.set_title("TÍN HIỆU ĐIỆN TIM THÔ (RAW AD8232)", color="#ff5252", fontsize=10, fontweight="bold")
        self.line_raw, = self.ax_raw.plot(self.raw_data, color="#ff5252", linewidth=1.2)
        self.ax_raw.tick_params(colors="#aaaaaa", labelsize=8)
        self.ax_raw.grid(True, color="#333333", linestyle="--")
        
        # Cấu hình nhãn biểu đồ lọc Notch
        self.ax_filt.set_title("TÍN HIỆU SẠCH SAU LỌC NOTCH 50Hz (CLEAN SIGNAL)", color="#00e676", fontsize=10, fontweight="bold")
        self.line_filt, = self.ax_filt.plot(self.filt_data, color="#00e676", linewidth=1.5)
        self.ax_filt.tick_params(colors="#aaaaaa", labelsize=8)
        self.ax_filt.grid(True, color="#333333", linestyle="--")
        
        self.fig.tight_layout()
        
        # Tích hợp Canvas của matplotlib vào Tkinter
        self.canvas = FigureCanvasTkAgg(self.fig, master=left_frame)
        self.canvas.get_tk_widget().pack(fill="both", expand=True)

        # --- CỘT PHẢI: BẢNG AI & ĐIỀU KHIỂN ---
        right_frame = tk.Frame(main_frame, bg="#121212", width=380)
        right_frame.pack(side="right", fill="both", padx=(10, 0))
        right_frame.pack_propagate(False)
        
        # Thẻ 1: Trạng thái & Kết nối
        conn_card = ttk.Frame(right_frame, style="Card.TFrame", padding=15)
        conn_card.pack(fill="x", pady=(0, 15))
        
        ttk.Label(conn_card, text="KẾT NỐI BLUETOOTH (BLE)", style="Title.TLabel").pack(anchor="w", pady=(0, 10))
        
        self.lbl_status = ttk.Label(conn_card, text="🔴 Chưa kết nối", style="Status.TLabel")
        self.lbl_status.pack(anchor="w", pady=(0, 10))
        
        # Dropdown lựa chọn thiết bị
        ttk.Label(conn_card, text="Chọn thiết bị hoặc địa chỉ MAC:", font=("Inter", 9), background="#1e1e1e", foreground="#aaaaaa").pack(anchor="w", pady=(0, 3))
        self.cb_devices = ttk.Combobox(conn_card, state="readonly", font=("Inter", 9))
        self.cb_devices.pack(fill="x", pady=(0, 12))
        
        # Mặc định thêm tùy chọn Tự động quét mặc định vào đầu danh sách
        default_opt = f"{DEVICE_NAME} [Tự động quét]"
        self.cb_devices["values"] = [default_opt]
        self.cb_devices.current(0)
        
        # Frame chứa nút Quét và nút Kết nối
        btn_layout = tk.Frame(conn_card, bg="#1e1e1e")
        btn_layout.pack(fill="x")
        
        self.btn_scan = ttk.Button(btn_layout, text="QUÉT THIẾT BỊ", style="Scan.TButton", command=self.start_scan)
        self.btn_scan.pack(side="left", fill="x", expand=True, padx=(0, 5))
        
        self.btn_connect = ttk.Button(btn_layout, text="KẾT NỐI", style="TButton", command=self.start_connection)
        self.btn_connect.pack(side="right", fill="x", expand=True, padx=(5, 0))
        
        # Thẻ 2: Nhịp tim & Chỉ số Sinh hiệu (ECG + SpO2 MAX30102)
        led_card = ttk.Frame(right_frame, style="Card.TFrame", padding=15)
        led_card.pack(fill="x", pady=(0, 15))
        
        ttk.Label(led_card, text="SINH HIỆU THỜI GIAN THỰC", style="Title.TLabel").pack(anchor="w", pady=(0, 8))
        
        # ECG BPM
        ecg_layout = tk.Frame(led_card, bg="#1e1e1e")
        ecg_layout.pack(fill="x", pady=(0, 5))
        tk.Label(ecg_layout, text="ECG BPM:", font=("Inter", 10), fg="#aaaaaa", bg="#1e1e1e").pack(side="left")
        self.lbl_bpm = tk.Label(ecg_layout, text="0.0", font=("Courier New", 28, "bold"), fg="#ff5252", bg="#1e1e1e")
        self.lbl_bpm.pack(side="right", padx=5)

        # MAX30102: SpO2 (%)
        spo2_layout = tk.Frame(led_card, bg="#1e1e1e")
        spo2_layout.pack(fill="x", pady=2)
        tk.Label(spo2_layout, text="SpO2 (MAX30102):", font=("Inter", 10), fg="#aaaaaa", bg="#1e1e1e").pack(side="left")
        self.lbl_spo2 = tk.Label(spo2_layout, text="-- %", font=("Courier New", 22, "bold"), fg="#00e5ff", bg="#1e1e1e")
        self.lbl_spo2.pack(side="right", padx=5)

        # MAX30102: PPG Pulse BPM
        ppg_layout = tk.Frame(led_card, bg="#1e1e1e")
        ppg_layout.pack(fill="x", pady=2)
        tk.Label(ppg_layout, text="PPG Pulse Rate:", font=("Inter", 10), fg="#aaaaaa", bg="#1e1e1e").pack(side="left")
        self.lbl_ppg_bpm = tk.Label(ppg_layout, text="--", font=("Courier New", 20, "bold"), fg="#ffab00", bg="#1e1e1e")
        self.lbl_ppg_bpm.pack(side="right", padx=5)
        
        # Thẻ 3: Chẩn đoán lâm sàng AI
        ai_card = ttk.Frame(right_frame, style="Card.TFrame", padding=15)
        ai_card.pack(fill="both", expand=True)
        
        ttk.Label(ai_card, text="CHẨN ĐOÁN LÂM SÀNG AI", style="Title.TLabel").pack(anchor="w", pady=(0, 15))
        
        # Phân loại hiện tại
        class_layout = tk.Frame(ai_card, bg="#1e1e1e")
        class_layout.pack(fill="x", pady=(0, 15))
        tk.Label(class_layout, text="Phân loại nhịp:", font=("Inter", 11), fg="#aaaaaa", bg="#1e1e1e").pack(side="left")
        self.lbl_class = tk.Label(class_layout, text="NORMAL (N)", font=("Outfit", 14, "bold"), fg="#00e676", bg="#1e1e1e")
        self.lbl_class.pack(side="right")
        
        # Phân bố xác suất 5 lớp
        ttk.Label(ai_card, text="Phần trăm xác suất phân lớp:", style="Status.TLabel").pack(anchor="w", pady=(5, 5))
        
        classes = ["N - Nhịp bình thường", "S - Trên thất đi sớm", "V - Ngoại tâm thu thất", "F - Nhịp hòa trộn", "Q - Chưa xác định"]
        self.prob_bars = []
        self.prob_labels = []
        
        for c in classes:
            row_frame = tk.Frame(ai_card, bg="#1e1e1e")
            row_frame.pack(fill="x", pady=3)
            
            lbl = tk.Label(row_frame, text=c, font=("Inter", 9), fg="#cccccc", bg="#1e1e1e")
            lbl.pack(side="left")
            
            val_lbl = tk.Label(row_frame, text="0%", font=("Inter", 9, "bold"), fg="#ffffff", bg="#1e1e1e")
            val_lbl.pack(side="right")
            self.prob_labels.append(val_lbl)
            
            bar = ttk.Progressbar(ai_card, orient="horizontal", length=300, mode="determinate")
            bar.pack(fill="x", pady=(0, 8))
            self.prob_bars.append(bar)
            
    # --- LUỒNG QUÉT THIẾT BỊ BLE ---
    def start_scan(self):
        self.btn_scan.configure(state="disabled", text="ĐANG QUÉT...")
        self.lbl_status.configure(text="🟡 Đang quét tìm thiết bị xung quanh (3s)...")
        self.scan_thread = threading.Thread(target=self.run_scan_loop, daemon=True)
        self.scan_thread.start()
        
    def run_scan_loop(self):
        loop = asyncio.new_event_loop()
        asyncio.set_event_loop(loop)
        loop.run_until_complete(self.scan_ble_devices())
        
    async def scan_ble_devices(self):
        try:
            devices = await BleakScanner.discover(timeout=3.0)
            device_list = []
            new_discovered = {}
            
            for d in devices:
                name = d.name if d.name else "Unknown Device"
                addr = d.address
                formatted = f"{name} [{addr}]"
                device_list.append(formatted)
                new_discovered[formatted] = addr
                
            self.root.after(0, self.on_scan_complete, device_list, new_discovered)
        except Exception as e:
            self.root.after(0, self.on_scan_complete, [], {})
            
    def on_scan_complete(self, device_list, new_discovered):
        self.btn_scan.configure(state="normal", text="QUÉT THIẾT BỊ")
        self.lbl_status.configure(text="🔴 Đã quét xong. Chọn thiết bị và kết nối.")
        
        # Thêm lại tùy chọn tự động quét mặc định vào đầu danh sách
        auto_opt = f"{DEVICE_NAME} [Tự động quét]"
        device_list.insert(0, auto_opt)
        
        self.cb_devices["values"] = device_list
        self.discovered_devices = new_discovered
        
        # Tự động chọn ca ESP32 mặc định nếu thấy
        selected_idx = 0
        for idx, item in enumerate(device_list):
            if DEVICE_NAME in item and "Tự động quét" not in item:
                selected_idx = idx
                break
        self.cb_devices.current(selected_idx)

    # --- LUỒNG KẾT NỐI VÀ NHẬN DỮ LIỆU ---
    def start_connection(self):
        if self.is_connected:
            self.stop_connection()
            return
            
        selected_str = self.cb_devices.get()
        if "Tự động quét" in selected_str:
            self.selected_mac = None # Kích hoạt tự động quét theo tên
        else:
            self.selected_mac = self.discovered_devices.get(selected_str)
            
        self.btn_connect.configure(text="ĐANG KẾT NỐI...", state="disabled")
        self.btn_scan.configure(state="disabled")
        self.lbl_status.configure(text="🟡 Đang thiết lập kết nối...")
        
        self.client_thread = threading.Thread(target=self.run_ble_loop, daemon=True)
        self.client_thread.start()
        
    def stop_connection(self):
        self.is_connected = False
        if self.loop and self.client:
            self.loop.call_soon_threadsafe(asyncio.create_task, self.disconnect_ble())
            
    async def disconnect_ble(self):
        if self.client:
            await self.client.disconnect()
            
    def run_ble_loop(self):
        self.loop = asyncio.new_event_loop()
        asyncio.set_event_loop(self.loop)
        try:
            self.loop.run_until_complete(self.ble_client_async())
        except Exception as e:
            print(f"Lỗi vòng lặp BLE: {e}")
            
    async def ble_client_async(self):
        target_device = None
        
        # 1. Tìm kiếm địa chỉ
        if self.selected_mac:
            target_device = self.selected_mac
            print(f"🔗 Kết nối trực tiếp đến MAC: {target_device}")
        else:
            print(f"🔍 Tự động quét tìm thiết bị có tên: {DEVICE_NAME}")
            device = await BleakScanner.find_device_by_filter(
                lambda d, ad: d.name == DEVICE_NAME
            )
            if device:
                target_device = device.address
                
        if not target_device:
            self.root.after(0, self.on_connection_failed, f"Không tìm thấy thiết bị '{DEVICE_NAME}'")
            return
            
        # 2. Kết nối tới Client
        self.client = BleakClient(target_device)
        try:
            await self.client.connect()
            
            # Cấu hình cờ trạng thái trước
            self.is_connected = True
            self.root.after(0, self.on_connected_success)
            
            # Đăng ký thông báo
            await self.client.start_notify(CHARACTERISTIC_UUID, self.ble_notification_callback)
            
            # Chờ 1 giây để kết nối hoàn toàn ổn định trên OS (Tránh Race Condition)
            await asyncio.sleep(1.0)
            
            # Vòng lặp duy trì kết nối (Kiểm tra kép bằng cả cờ nội bộ và Bleak)
            while self.is_connected and self.client.is_connected:
                await asyncio.sleep(0.2)
                
        except Exception as e:
            self.root.after(0, self.on_connection_failed, str(e))
        finally:
            self.is_connected = False
            self.root.after(0, self.on_disconnected)

    def ble_notification_callback(self, sender, data):
        try:
            packet_str = data.decode("utf-8")
            if packet_str.startswith("DBG:"):
                print(f"[ESP32 Debug]: {packet_str[4:]}")
                return
                
            parts = packet_str.split("|")
            fields = {}
            for part in parts:
                if ":" in part:
                    k, v = part.split(":", 1)
                    fields[k] = v
                    
            raw_val = float(fields.get("R", 0.0))
            filt_val = float(fields.get("F", 0.0))
            is_peak = int(fields.get("P", 0))
            
            # Đưa vào hàng đợi
            self.data_queue.put((raw_val, filt_val, is_peak, fields))
        except Exception as e:
            pass

    def on_connected_success(self):
        self.btn_connect.configure(text="NGẮT KẾT NỐI", style="Disconnect.TButton", state="normal")
        self.btn_scan.configure(state="disabled")
        self.lbl_status.configure(text="🟢 Đã kết nối thành công!")
        
    def on_connection_failed(self, err_msg):
        self.btn_connect.configure(text="KẾT NỐI", style="TButton", state="normal")
        self.btn_scan.configure(state="normal")
        self.lbl_status.configure(text="🔴 Kết nối lỗi!")
        messagebox.showerror("Lỗi kết nối", f"Không thể kết nối BLE: {err_msg}")
        
    def on_disconnected(self):
        self.btn_connect.configure(text="KẾT NỐI", style="TButton", state="normal")
        self.btn_scan.configure(state="normal")
        self.lbl_status.configure(text="🔴 Chưa kết nối")
        self.lbl_bpm.configure(text="0.0")
        self.lbl_spo2.configure(text="-- %")
        self.lbl_ppg_bpm.configure(text="--")

    # --- VÒNG LẶP CẬP NHẬT GUI (16ms = ~60 FPS) ---
    def update_gui_loop(self):
        samples_processed = 0
        new_raws = []
        new_filts = []
        peak_detected = False
        last_fields = {}
        
        while not self.data_queue.empty() and samples_processed < 50:
            raw, filt, is_peak, fields = self.data_queue.get_nowait()
            new_raws.append(raw)
            new_filts.append(filt)
            
            if is_peak == 1:
                peak_detected = True
                last_fields = fields

            # Cập nhật thông số SpO2 và PPG từ MAX30102
            if "SPO2" in fields:
                try:
                    s_val = float(fields["SPO2"])
                    if s_val > 0:
                        self.last_spo2 = s_val
                        self.lbl_spo2.configure(text=f"{s_val:.1f} %")
                    else:
                        self.lbl_spo2.configure(text="-- %")
                except ValueError:
                    pass
            if "PBPM" in fields:
                try:
                    p_val = float(fields["PBPM"])
                    if p_val > 0:
                        self.last_ppg_bpm = p_val
                        self.lbl_ppg_bpm.configure(text=f"{p_val:.1f}")
                    else:
                        self.lbl_ppg_bpm.configure(text="--")
                except ValueError:
                    pass
                
            # Trích xuất xác suất lớp
            prob_list = [0.0, 0.0, 0.0, 0.0, 0.0]
            if "Q" in fields:
                prob_list = [float(p) for p in fields["Q"].split(",")]
                self.last_probs = prob_list
                
            # Lưu log CSV liên tục
            self.csv_writer.writerow([
                self.sample_index,
                raw,
                filt,
                is_peak,
                float(fields.get("B", self.last_bpm)) if is_peak else 0.0,
                int(fields.get("C", 0)) if is_peak else 0,
                fields.get("CN", "N") if is_peak else "N",
                prob_list[0], prob_list[1], prob_list[2], prob_list[3], prob_list[4]
            ])
            self.sample_index += 1
            samples_processed += 1
            
        # Cập nhật đồ thị và phần trăm hiển thị
        if samples_processed > 0:
            self.raw_data = np.roll(self.raw_data, -samples_processed)
            self.filt_data = np.roll(self.filt_data, -samples_processed)
            
            self.raw_data[-samples_processed:] = new_raws
            self.filt_data[-samples_processed:] = new_filts
            
            self.line_raw.set_ydata(self.raw_data)
            self.line_filt.set_ydata(self.filt_data)
            
            self.ax_raw.set_ylim(np.min(self.raw_data) - 100, np.max(self.raw_data) + 100)
            self.ax_filt.set_ylim(np.min(self.filt_data) - 100, np.max(self.filt_data) + 100)
            
            self.canvas.draw_idle()
            
            # Cập nhật các Progress bar xác suất
            for idx in range(5):
                p_val = self.last_probs[idx]
                self.prob_bars[idx]["value"] = p_val
                self.prob_labels[idx].configure(text=f"{p_val:.0f}%")
                
        # Khi phát hiện nhịp tim (Peak)
        if peak_detected:
            self.last_bpm = float(last_fields.get("B", self.last_bpm))
            self.last_class = last_fields.get("CN", "N")
            
            # Cập nhật BPM hiển thị lên giao diện (Tránh bị ghi đè về 0 khi đang kết nối)
            if self.is_connected:
                self.lbl_bpm.configure(text=f"{self.last_bpm:.1f}")
            
            # Trích xuất ký tự đầu tiên để đổi màu (Ví dụ: "N" từ "N - Normal Beat")
            class_key = self.last_class[0] if self.last_class else "N"
            class_colors = {"N": "#00e676", "S": "#ffeb3b", "V": "#ff1744", "F": "#29b6f6", "Q": "#9c27b0"}
            clr = class_colors.get(class_key, "#ffffff")
            
            # Tránh lặp chữ "Beat" nếu trong chuỗi đã có sẵn
            display_text = self.last_class
            if "Beat" not in display_text:
                display_text = f"{display_text} Beat"
                
            self.lbl_class.configure(text=display_text, fg=clr)
            
        # Lặp lại sau 16ms
        self.root.after(16, self.update_gui_loop)

    def on_closing(self):
        self.stop_connection()
        self.csv_file.close()
        self.root.destroy()

if __name__ == "__main__":
    root = tk.Tk()
    app = BLE_ECG_GUI(root)
    root.protocol("WM_DELETE_WINDOW", app.on_closing)
    root.mainloop()
