"""
Windows標準カーソルファイル(.cur)を読み込み、グローを追加するスクリプト
"""
from PIL import Image, ImageDraw, ImageFilter
import struct
import os

OUTPUT_DIR = os.path.dirname(os.path.abspath(__file__))

def read_cur_file(cur_path: str):
    """
    .curファイルを読み込んでPIL Imageを返す
    """
    with open(cur_path, 'rb') as f:
        # ICONDIRヘッダー (6 bytes)
        reserved, image_type, image_count = struct.unpack('<HHH', f.read(6))
        
        if image_count == 0:
            raise ValueError("No images in cursor file")
        
        # 最初のエントリを読む (16 bytes)
        width = struct.unpack('<B', f.read(1))[0]
        height = struct.unpack('<B', f.read(1))[0]
        color_count = struct.unpack('<B', f.read(1))[0]
        reserved2 = struct.unpack('<B', f.read(1))[0]
        hotspot_x = struct.unpack('<H', f.read(2))[0]
        hotspot_y = struct.unpack('<H', f.read(2))[0]
        size_in_bytes = struct.unpack('<I', f.read(4))[0]
        offset = struct.unpack('<I', f.read(4))[0]
        
        # 0は256を意味する
        if width == 0:
            width = 256
        if height == 0:
            height = 256
        
        # 画像データを読む
        f.seek(offset)
        
        # BITMAPINFOHEADER
        bi_size = struct.unpack('<I', f.read(4))[0]
        bi_width = struct.unpack('<i', f.read(4))[0]
        bi_height = struct.unpack('<i', f.read(4))[0]
        bi_planes = struct.unpack('<H', f.read(2))[0]
        bi_bit_count = struct.unpack('<H', f.read(2))[0]
        bi_compression = struct.unpack('<I', f.read(4))[0]
        f.read(20)  # 残りのヘッダーをスキップ
        
        actual_height = abs(bi_height) // 2  # XOR + AND
        
        if bi_bit_count == 32:
            # 32bit BGRA
            pixel_data = f.read(bi_width * actual_height * 4)
            
            # PIL Imageに変換
            img = Image.new('RGBA', (bi_width, actual_height))
            pixels = []
            
            for y in range(actual_height - 1, -1, -1):  # ボトムアップ
                for x in range(bi_width):
                    idx = (y * bi_width + x) * 4
                    b = pixel_data[idx]
                    g = pixel_data[idx + 1]
                    r = pixel_data[idx + 2]
                    a = pixel_data[idx + 3]
                    pixels.append((r, g, b, a))
            
            img.putdata(pixels)
            return img, hotspot_x, hotspot_y
        
        elif bi_bit_count == 1:
            # 1bit モノクロカーソル (XOR + AND マスク)
            row_size = ((bi_width + 31) // 32) * 4  # 4バイト境界
            
            # XORマスク (画像データ)
            xor_data = f.read(row_size * actual_height)
            # ANDマスク (透明マスク)
            and_data = f.read(row_size * actual_height)
            
            img = Image.new('RGBA', (bi_width, actual_height))
            pixels = []
            
            for y in range(actual_height - 1, -1, -1):  # ボトムアップ
                for x in range(bi_width):
                    byte_idx = x // 8
                    bit_idx = 7 - (x % 8)
                    row_offset = y * row_size
                    
                    xor_bit = (xor_data[row_offset + byte_idx] >> bit_idx) & 1
                    and_bit = (and_data[row_offset + byte_idx] >> bit_idx) & 1
                    
                    # AND=0, XOR=0 → 黒 (不透明)
                    # AND=0, XOR=1 → 白 (不透明)
                    # AND=1, XOR=0 → 透明
                    # AND=1, XOR=1 → 反転 (半透明白として表現)
                    if and_bit == 0:
                        if xor_bit == 0:
                            pixels.append((0, 0, 0, 255))  # 黒
                        else:
                            pixels.append((255, 255, 255, 255))  # 白
                    else:
                        if xor_bit == 0:
                            pixels.append((0, 0, 0, 0))  # 透明
                        else:
                            pixels.append((128, 128, 128, 180))  # 反転→半透明グレー
            
            img.putdata(pixels)
            return img, hotspot_x, hotspot_y
        
        else:
            raise ValueError(f"Unsupported bit count: {bi_bit_count}")


def create_ibeam_cursor(size: int = 32):
    """
    I字型カーソル（テキスト入力用）を手動作成
    Windows標準 ibeam.svg の座標に基づく:
    - 上セリフ: X 11-21, Y 7.5-9.5 (幅10, 高さ2)
    - 縦棒: X 15-17, Y 9.5-22.5 (幅2, 高さ13)
    - 下セリフ: X 11-21, Y 22.5-24.5 (幅10, 高さ2)
    """
    img = Image.new('RGBA', (size, size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    
    # 白い塗り（#FFFFFE）+ 黒い枠線（#000001）
    # SVGのpolygon座標をそのまま使用
    points = [
        (21, 9), (21, 7), (17, 7), (17, 8), (15, 8), (15, 7), (11, 7), (11, 9),
        (15, 9), (15, 22), (11, 22), (11, 24), (15, 24), (15, 24), (17, 24), (17, 24),
        (21, 24), (21, 22), (17, 22), (17, 9)
    ]
    
    # 白塗り + 黒枠
    draw.polygon(points, fill=(255, 255, 255, 255), outline=(0, 0, 0, 255))
    
    return img


def create_wait_cursor(size: int = 32):
    """
    砂時計カーソル（待機用）を手動作成
    Windows標準 wait.svg の座標に基づく
    """
    img = Image.new('RGBA', (size, size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    
    # 砂時計本体（白塗り）
    hourglass_points = [
        (10, 25), (22, 25), (22, 21), (18, 17), (18, 15), (22, 11), (22, 6),
        (10, 6), (10, 11), (14, 15), (14, 17), (10, 21)
    ]
    draw.polygon(hourglass_points, fill=(255, 255, 255, 255))
    
    # 輪郭線（黒）
    draw.line([(22, 25), (22, 21), (18, 17), (18, 15), (22, 11), (22, 6)], fill=(0, 0, 0, 255))
    draw.line([(10, 6), (10, 11), (14, 15), (14, 17), (10, 21), (10, 25)], fill=(0, 0, 0, 255))
    
    # 上下の枠（白塗り＋黒枠）
    draw.rectangle([9, 4, 23, 6], fill=(255, 255, 255, 255), outline=(0, 0, 0, 255))
    draw.rectangle([9, 25, 23, 27], fill=(255, 255, 255, 255), outline=(0, 0, 0, 255))
    
    return img


def create_cross_cursor(size: int = 32):
    """
    十字カーソル（精密選択用）を手動作成
    Windows標準 cross.svg の座標に基づく:
    - 縦線: X 15-17, Y 7-25 (幅2, 高さ18)
    - 横線: X 7-25, Y 15-17 (幅18, 高さ2)
    """
    img = Image.new('RGBA', (size, size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    
    # SVGのpolygon座標に基づく十字形
    points = [
        (15, 15), (15, 7), (17, 7), (17, 15), (25, 15), (25, 17),
        (17, 17), (17, 25), (15, 25), (15, 17), (7, 17), (7, 15)
    ]
    
    # 白塗り + 黒枠
    draw.polygon(points, fill=(255, 255, 255, 255), outline=(0, 0, 0, 255))
    
    return img


def add_glow_to_cursor(cursor_img: Image.Image, glow_color: tuple, glow_size: int = 3, heavy_style: bool = True):
    """
    カーソル画像にグローを追加
    heavy_style=True: Heavyグロー（右下強調）
    heavy_style=False: Lightグロー（対称・全方向均等）
    """
    width, height = cursor_img.size
    
    # カーソル画像を32x32にリサイズ（大きすぎる場合）
    if width > 32 or height > 32:
        cursor_img = cursor_img.resize((32, 32), Image.Resampling.LANCZOS)
        width, height = 32, 32
    
    # グロー用に大きなキャンバス
    padding = glow_size + 2
    if heavy_style:
        extra = 4  # 右下に追加余白
        canvas_w = width + padding * 2 + extra
        canvas_h = height + padding * 2 + extra
    else:
        canvas_w = width + padding * 2
        canvas_h = height + padding * 2
    
    # アルファチャンネルを取得
    if cursor_img.mode == 'RGBA':
        alpha = cursor_img.split()[3]
    else:
        alpha = Image.new('L', cursor_img.size, 255)
    
    # === 通常グロー層（対称） ===
    glow_img = Image.new('RGBA', (canvas_w, canvas_h), (0, 0, 0, 0))
    alpha_padded = Image.new('L', (canvas_w, canvas_h), 0)
    alpha_padded.paste(alpha, (padding, padding))
    
    for i in range(glow_size, 0, -1):
        glow_alpha = int(80 * (1 - i / (glow_size + 2)))
        expanded = alpha_padded.filter(ImageFilter.MaxFilter(i * 2 + 1))
        glow_layer = Image.new('RGBA', (canvas_w, canvas_h), (*glow_color, 0))
        glow_mask = expanded.point(lambda x: glow_alpha if x > 50 else 0)
        glow_layer.putalpha(glow_mask)
        glow_img = Image.alpha_composite(glow_img, glow_layer)
    
    glow_img = glow_img.filter(ImageFilter.GaussianBlur(radius=1.5))
    
    # === 右下追加グロー層（heavy_styleの場合のみ） ===
    if heavy_style:
        extra_glow = Image.new('RGBA', (canvas_w, canvas_h), (0, 0, 0, 0))
        alpha_offset = Image.new('L', (canvas_w, canvas_h), 0)
        alpha_offset.paste(alpha, (padding + 2, padding + 2))  # 右下にオフセット
        
        for i in range(glow_size + 1, 0, -1):
            glow_alpha = int(60 * (1 - i / (glow_size + 3)))
            expanded = alpha_offset.filter(ImageFilter.MaxFilter(i * 2 + 1))
            glow_layer = Image.new('RGBA', (canvas_w, canvas_h), (*glow_color, 0))
            glow_mask = expanded.point(lambda x: glow_alpha if x > 50 else 0)
            glow_layer.putalpha(glow_mask)
            extra_glow = Image.alpha_composite(extra_glow, glow_layer)
        
        extra_glow = extra_glow.filter(ImageFilter.GaussianBlur(radius=2.0))
        
        # 通常グロー + 右下追加グローを合成
        glow_img = Image.alpha_composite(glow_img, extra_glow)
    
    # 元のカーソルを上に合成
    result = Image.new('RGBA', (canvas_w, canvas_h), (0, 0, 0, 0))
    result = Image.alpha_composite(result, glow_img)
    result.paste(cursor_img, (padding, padding), cursor_img)
    
    # 32x32にクロップ
    crop_left = padding - 2
    crop_top = padding - 2
    final = result.crop((crop_left, crop_top, crop_left + 32, crop_top + 32))
    
    return final


def png_to_cur(img: Image.Image, cur_path: str, hotspot_x: int = 0, hotspot_y: int = 0):
    """
    PIL ImageをCURファイルとして保存
    """
    width, height = img.size
    
    if img.mode != 'RGBA':
        img = img.convert('RGBA')
    
    pixels = list(img.getdata())
    
    # BITMAPINFOHEADER
    bmp_header = struct.pack('<I', 40)
    bmp_header += struct.pack('<i', width)
    bmp_header += struct.pack('<i', height * 2)
    bmp_header += struct.pack('<H', 1)
    bmp_header += struct.pack('<H', 32)
    bmp_header += struct.pack('<I', 0)
    bmp_header += struct.pack('<I', 0)
    bmp_header += struct.pack('<i', 0)
    bmp_header += struct.pack('<i', 0)
    bmp_header += struct.pack('<I', 0)
    bmp_header += struct.pack('<I', 0)
    
    # XORデータ (BGRA、ボトムアップ)
    xor_data = bytearray()
    for y in range(height - 1, -1, -1):
        for x in range(width):
            r, g, b, a = pixels[y * width + x]
            xor_data.extend([b, g, r, a])
    
    # ANDマスク
    and_row_size = ((width + 31) // 32) * 4
    and_data = bytearray()
    for y in range(height - 1, -1, -1):
        row = bytearray(and_row_size)
        for x in range(width):
            _, _, _, a = pixels[y * width + x]
            if a < 128:
                byte_idx = x // 8
                bit_idx = 7 - (x % 8)
                row[byte_idx] |= (1 << bit_idx)
        and_data.extend(row)
    
    icon_data = bmp_header + bytes(xor_data) + bytes(and_data)
    
    cur_header = struct.pack('<HHH', 0, 2, 1)
    entry = struct.pack('<BBBBHHII',
        width if width < 256 else 0,
        height if height < 256 else 0,
        0, 0,
        hotspot_x, hotspot_y,
        len(icon_data), 22
    )
    
    with open(cur_path, 'wb') as f:
        f.write(cur_header)
        f.write(entry)
        f.write(icon_data)
    
    print(f"Generated: {cur_path}")


# 全システムカーソル定義
# (名前, ファイル名, デフォルトhotspot_x, デフォルトhotspot_y)
# 注: _r.cur は1ビット形式で非対応なので、_m.cur（32ビット）を使用
SYSTEM_CURSORS = [
    ("arrow", "aero_arrow.cur", 0, 0),
    ("ibeam", "beam_m.cur", 4, 9),           # beam_r.curは1bit、beam_m.curは32bit
    ("wait", "wait_m.cur", 16, 16),          # aero_busy.curは.ani、wait_m.curは.cur
    ("cross", "cross_m.cur", 16, 16),        # cross_r.curは1bit、cross_m.curは32bit
    ("uparrow", "aero_up.cur", 5, 0),
    ("sizenwse", "aero_nwse.cur", 8, 8),
    ("sizenesw", "aero_nesw.cur", 8, 8),
    ("sizewe", "aero_ew.cur", 8, 8),
    ("sizens", "aero_ns.cur", 8, 8),
    ("sizeall", "aero_move.cur", 16, 16),
    ("no", "aero_unavail.cur", 8, 8),
    ("hand", "aero_link.cur", 6, 0),
    ("appstarting", "aero_working.ani", 0, 0),  # .aniは非対応、スキップ
]

def save_preview_image(cursor_img: Image.Image, preview_path: str, label: str = ""):
    """
    README用のプレビュー画像を生成（暗い背景でグローが見えやすいように）
    """
    # 背景サイズ（カーソルより大きくしてグローが見える余白を確保）
    bg_size = 64
    bg_color = (30, 30, 35)  # ダークグレー背景
    
    preview = Image.new('RGBA', (bg_size, bg_size), (*bg_color, 255))
    
    # カーソルを中央に配置
    cursor_w, cursor_h = cursor_img.size
    offset_x = (bg_size - cursor_w) // 2
    offset_y = (bg_size - cursor_h) // 2
    preview.paste(cursor_img, (offset_x, offset_y), cursor_img)
    
    # PNGとして保存
    preview.save(preview_path, 'PNG')
    print(f"  Preview: {preview_path}")


def process_cursor(name: str, filename: str, default_hotspot_x: int, default_hotspot_y: int) -> bool:
    """
    単一のカーソルを処理してEN/JP版を生成
    """
    # .aniファイルはスキップ
    if filename.endswith('.ani'):
        print(f"  Skipping animated cursor: {filename}")
        return False
    
    try:
        # 特定のカーソルは自作関数を使用（1ビット形式回避）
        if name == "ibeam":
            cursor_32 = create_ibeam_cursor(32)
            hotspot_x, hotspot_y = 16, 16  # 中央
        elif name == "wait":
            cursor_32 = create_wait_cursor(32)
            hotspot_x, hotspot_y = 16, 16  # 中央
        elif name == "cross":
            cursor_32 = create_cross_cursor(32)
            hotspot_x, hotspot_y = 16, 16  # 中央
        else:
            # システムカーソルから読み込み
            cursor_path = os.path.join(r"C:\Windows\Cursors", filename)
            
            if not os.path.exists(cursor_path):
                print(f"  Not found: {cursor_path}")
                return False
            
            cursor_img, hotspot_x, hotspot_y = read_cur_file(cursor_path)
            
            # hotspotが0,0の場合はデフォルト値を使用
            if hotspot_x == 0 and hotspot_y == 0:
                hotspot_x = default_hotspot_x
                hotspot_y = default_hotspot_y
            
            # 32x32にリサイズ（大きい場合）
            if cursor_img.size[0] > 32 or cursor_img.size[1] > 32:
                cursor_32 = cursor_img.resize((32, 32), Image.Resampling.LANCZOS)
                # hotspotも比率でスケール
                scale = 32 / max(cursor_img.size)
                hotspot_x = int(hotspot_x * scale)
                hotspot_y = int(hotspot_y * scale)
            else:
                cursor_32 = cursor_img
        
        # プレビュー画像用ディレクトリ
        images_dir = os.path.join(OUTPUT_DIR, "docs", "images")
        if not os.path.exists(images_dir):
            os.makedirs(images_dir)
        
        # === Lightグロー版（対称） ===
        # 青グロー (EN)
        blue_light = add_glow_to_cursor(cursor_32, (0, 150, 255), glow_size=3, heavy_style=False)
        en_light_path = os.path.join(OUTPUT_DIR, "resources", f"cursor_{name}_en_light.cur")
        png_to_cur(blue_light, en_light_path, hotspot_x=max(2, hotspot_x), hotspot_y=max(2, hotspot_y))
        save_preview_image(blue_light, os.path.join(images_dir, f"cursor_{name}_en_light.png"))
        
        # 赤グロー (JP)
        red_light = add_glow_to_cursor(cursor_32, (255, 50, 50), glow_size=3, heavy_style=False)
        jp_light_path = os.path.join(OUTPUT_DIR, "resources", f"cursor_{name}_jp_light.cur")
        png_to_cur(red_light, jp_light_path, hotspot_x=max(2, hotspot_x), hotspot_y=max(2, hotspot_y))
        save_preview_image(red_light, os.path.join(images_dir, f"cursor_{name}_jp_light.png"))
        
        # === Heavyグロー版（右下強調） ===
        # 青グロー (EN)
        blue_heavy = add_glow_to_cursor(cursor_32, (0, 150, 255), glow_size=3, heavy_style=True)
        en_heavy_path = os.path.join(OUTPUT_DIR, "resources", f"cursor_{name}_en_heavy.cur")
        png_to_cur(blue_heavy, en_heavy_path, hotspot_x=max(2, hotspot_x), hotspot_y=max(2, hotspot_y))
        save_preview_image(blue_heavy, os.path.join(images_dir, f"cursor_{name}_en_heavy.png"))
        
        # 赤グロー (JP)
        red_heavy = add_glow_to_cursor(cursor_32, (255, 50, 50), glow_size=3, heavy_style=True)
        jp_heavy_path = os.path.join(OUTPUT_DIR, "resources", f"cursor_{name}_jp_heavy.cur")
        png_to_cur(red_heavy, jp_heavy_path, hotspot_x=max(2, hotspot_x), hotspot_y=max(2, hotspot_y))
        save_preview_image(red_heavy, os.path.join(images_dir, f"cursor_{name}_jp_heavy.png"))
        
        return True
        
    except Exception as e:
        print(f"  Error processing {name}: {e}")
        return False


def main():
    print("=== 全システムカーソル生成スクリプト ===\n")
    
    # 出力ディレクトリ確認
    resources_dir = os.path.join(OUTPUT_DIR, "resources")
    if not os.path.exists(resources_dir):
        os.makedirs(resources_dir)
    
    success_count = 0
    skip_count = 0
    fail_count = 0
    
    for name, filename, hx, hy in SYSTEM_CURSORS:
        print(f"Processing: {name} ({filename})")
        result = process_cursor(name, filename, hx, hy)
        if result:
            success_count += 1
        elif filename.endswith('.ani'):
            skip_count += 1
        else:
            fail_count += 1
    
    print(f"\n=== 完了 ===")
    print(f"成功: {success_count}")
    print(f"スキップ: {skip_count}")
    print(f"失敗: {fail_count}")
    print(f"合計ファイル: {success_count * 4} (Light EN/JP + Heavy EN/JP)")


if __name__ == "__main__":
    main()
