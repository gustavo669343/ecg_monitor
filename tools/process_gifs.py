#!/usr/bin/env python3
"""
Tool: process_gifs.py
Cong dung:
1. Quet toan bo file .gif trong thu muc (hoac 1 file chi dinh).
2. Resize anh GIF theo kich thuoc mong muon (giu nguyen animation va ty le).
3. Toi uu hoa bang mau (Color Palette):
   - Chuyen ve 8-bit Indexed Color (toi da 256 mau, hoac 128 mau).
   - Xu ly nen trong suot (Transparency Index) chuan xac.
   - Dat Disposal Method = 2 (Restore to background) de tranh lem net frame cu.
4. Tu dong xuat ra file C++ Header (.h) trong include/ voi mang PROGMEM.
5. Tu dong cap nhat danh sach include/GifRegistry.h de co the goi theo ten!
"""

import os
import sys
import argparse
import glob

def process_single_gif(input_path, output_gif_path, target_width=None, target_height=None, max_colors=128):
    try:
        from PIL import Image, ImageSequence
    except ImportError:
        print("[ERROR] Thu vien Pillow chua duoc cai dat!")
        print("Hay chay lenh: pip install Pillow")
        return False

    print(f"\n[DANG XU LY] File: {input_path}")
    orig_img = Image.open(input_path)

    frames = []
    durations = []
    disposals = []

    # Tinh toan kich thuoc moi
    orig_w, orig_h = orig_img.size
    if target_width and target_height:
        new_w, new_h = target_width, target_height
    elif target_width:
        new_w = target_width
        new_h = int(orig_h * (target_width / orig_w))
    elif target_height:
        new_h = target_height
        new_w = int(orig_w * (target_height / orig_h))
    else:
        new_w, new_h = orig_w, orig_h

    print(f" -> Kich thuoc: {orig_w}x{orig_h} => {new_w}x{new_h}")

    for frame in ImageSequence.Iterator(orig_img):
        duration = frame.info.get('duration', 100)
        durations.append(duration)
        disposals.append(2) # Disposal method 2: Restore to background

        # Chuyen sang RGBA de giu kenh trong suot khi resize
        rgba_frame = frame.convert('RGBA')
        if (new_w, new_h) != (orig_w, orig_h):
            # Dung NEAREST neu la pixel art, hoac LANCZOS neu la anh thuong
            resized = rgba_frame.resize((new_w, new_h), Image.Resampling.NEAREST if orig_w <= 48 else Image.Resampling.LANCZOS)
        else:
            resized = rgba_frame

        # Toi uu bang mau (Quantize ve Indexed Color co transparency)
        alpha = resized.split()[-1]
        # Tao mask cho diem trong suot
        mask = Image.eval(alpha, lambda a: 255 if a <= 128 else 0)

        # Chuyen RGB sang bang mau max_colors
        rgb = resized.convert('RGB')
        p_frame = rgb.convert('P', palette=Image.Palette.ADAPTIVE, colors=max_colors - 1)

        # Dat mau trong suot vao index cuoi cung (max_colors - 1)
        trans_idx = max_colors - 1
        p_frame.paste(trans_idx, mask)
        p_frame.info['transparency'] = trans_idx

        frames.append(p_frame)

    if not frames:
        print(" -> Khong doc duoc frame nao!")
        return False

    # Luu file GIF toi uu
    os.makedirs(os.path.dirname(output_gif_path) or '.', exist_ok=True)
    frames[0].save(
        output_gif_path,
        save_all=True,
        append_images=frames[1:],
        duration=durations,
        loop=0,
        disposal=disposals,
        transparency=trans_idx,
        optimize=True
    )

    in_size = os.path.getsize(input_path)
    out_size = os.path.getsize(output_gif_path)
    print(f" -> Da xuat GIF: {output_gif_path} ({in_size} bytes => {out_size} bytes)")
    return True

def gif_to_c_header(gif_path, header_path, var_name):
    with open(gif_path, "rb") as f:
        bytes_data = f.read()

    lines = [
        "#pragma once",
        "#include <Arduino.h>",
        "",
        f"const uint8_t {var_name}[] PROGMEM = {{"
    ]

    chunk_size = 16
    for i in range(0, len(bytes_data), chunk_size):
        chunk = bytes_data[i:i + chunk_size]
        hex_str = ", ".join(f"0x{b:02x}" for b in chunk)
        if i + chunk_size < len(bytes_data):
            hex_str += ","
        lines.append(f"    {hex_str}")

    lines.append("};")
    lines.append(f"const size_t {var_name}_size = {len(bytes_data)};")
    lines.append("")

    os.makedirs(os.path.dirname(header_path), exist_ok=True)
    with open(header_path, "w", encoding="ascii") as f:
        f.write("\n".join(lines))

    print(f" -> Da tao Header C++: {header_path} ({len(bytes_data)} bytes PROGMEM)")

def update_registry(registry_file, gif_entries):
    """
    gif_entries: list of dict {'name': '...', 'var_name': '...', 'header_file': '...', 'default_scale': 6}
    """
    lines = [
        "#pragma once",
        "#include <Arduino.h>",
        ""
    ]

    if os.path.exists("include/SampleGif.h"):
        lines.append('#include "SampleGif.h"')

    for entry in gif_entries:
        lines.append(f'#include "{entry["header_file"]}"')

    lines.extend([
        "",
        "struct GifEntry {",
        "    const char* name;",
        "    const uint8_t* data;",
        "    size_t size;",
        "    uint8_t defaultScale;",
        "};",
        "",
        "const GifEntry GIF_REGISTRY[] = {"
    ])

    for entry in gif_entries:
        lines.append(f'    {{ "{entry["name"]}", {entry["var_name"]}, {entry["var_name"]}_size, {entry["default_scale"]} }},')

    if os.path.exists("include/SampleGif.h"):
        lines.append('    { "star", sample_star_gif, sample_star_gif_size, 8 },')

    lines.extend([
        "};",
        "",
        "const size_t GIF_REGISTRY_COUNT = sizeof(GIF_REGISTRY) / sizeof(GIF_REGISTRY[0]);",
        "",
        "inline const GifEntry* findGifByName(const char* name) {",
        "    if (!name) return nullptr;",
        "    for (size_t i = 0; i < GIF_REGISTRY_COUNT; i++) {",
        "        if (strcasecmp(GIF_REGISTRY[i].name, name) == 0) {",
        "            return &GIF_REGISTRY[i];",
        "        }",
        "    }",
        "    return nullptr;",
        "}",
        ""
    ])

    with open(registry_file, "w", encoding="utf-8") as f:
        f.write("\n".join(lines))

    print(f"\n[THANH CONG] Da cap nhat Registry: {registry_file} voi {len(gif_entries)} GIF!")

def main():
    parser = argparse.ArgumentParser(description="Tool toi uu hoa va convert GIF cho ESP32 TFT_eSPI")
    parser.add_argument("-i", "--input", default=".", help="File .gif hoac thu muc chua cac file .gif (mac dinh: .)")
    parser.add_argument("-w", "--width", type=int, default=96, help="Chieu rong mong muon (mac dinh: 96px)")
    parser.add_argument("-H", "--height", type=int, default=96, help="Chieu cao mong muon (mac dinh: 96px)")
    parser.add_argument("-c", "--colors", type=int, default=128, help="So mau toi da trong bang mau (mac dinh: 128)")
    parser.add_argument("-s", "--scale", type=int, default=2, help="Ty le phong to mac dinh tren man hinh (mac dinh: 2x cho 96x96 -> 192x192)")


    args = parser.parse_args()

    # Tim tat ca cac file .gif
    if os.path.isfile(args.input):
        gif_files = [args.input]
    elif os.path.isdir(args.input):
        gif_files = glob.glob(os.path.join(args.input, "*.gif"))
    else:
        gif_files = glob.glob("*.gif")

    if not gif_files:
        print(f"[THONG BAO] Khong tim thay file .gif nao trong '{args.input}'!")
        print("Hay tao thu muc 'gifs/' va chep cac file .gif vao do.")
        return

    registry_entries = []

    for fpath in gif_files:
        base_name = os.path.splitext(os.path.basename(fpath))[0].lower()
        clean_name = "".join(c if c.isalnum() else "_" for c in base_name).strip("_")
        
        opt_gif_path = os.path.join("gifs_optimized", f"{clean_name}.gif")
        header_name = f"{clean_name.capitalize()}Gif.h"
        header_path = os.path.join("include", header_name)
        var_name = f"{clean_name}_gif"

        # 1. Resize & Quantize
        ok = process_single_gif(fpath, opt_gif_path, args.width, args.height, args.colors)
        target_gif = opt_gif_path if ok else fpath

        # 2. Convert to C++ Header
        gif_to_c_header(target_gif, header_path, var_name)

        # 3. Add to registry
        registry_entries.append({
            "name": clean_name,
            "var_name": var_name,
            "header_file": header_name,
            "default_scale": args.scale
        })

    # 4. Cap nhat include/GifRegistry.h
    update_registry("include/GifRegistry.h", registry_entries)

if __name__ == "__main__":
    main()
