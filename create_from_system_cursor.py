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
        
        # BITMAPINFOヘADER
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
        else:
            raise ValueError(f"Unsupported bit count: {bi_bit_count}")


def add_glow_to_cursor(cursor_img: Image.Image, glow_color: tuple, glow_size: int = 3):
    """
    カーソル画像にグローを追加
    """
    width, height = cursor_img.size
    
    # カーソル画像を32x32にリサイズ（大きすぎる場合）
    if width > 32 or height > 32:
        cursor_img = cursor_img.resize((32, 32), Image.Resampling.LANCZOS)
        width, height = 32, 32
    
    # グロー用に大きなキャンバス
    padding = glow_size + 2
    canvas_w = width + padding * 2
    canvas_h = height + padding * 2
    
    # アルファチャンネルを取得
    if cursor_img.mode == 'RGBA':
        alpha = cursor_img.split()[3]
    else:
        alpha = Image.new('L', cursor_img.size, 255)
    
    # グロー層を作成
    glow_img = Image.new('RGBA', (canvas_w, canvas_h), (0, 0, 0, 0))
    
    # アルファをパディング付きで配置
    alpha_padded = Image.new('L', (canvas_w, canvas_h), 0)
    alpha_padded.paste(alpha, (padding, padding))
    
    # 複数レイヤーでグロー効果
    for i in range(glow_size, 0, -1):
        glow_alpha = int(80 * (1 - i / (glow_size + 2)))
        expanded = alpha_padded.filter(ImageFilter.MaxFilter(i * 2 + 1))
        glow_layer = Image.new('RGBA', (canvas_w, canvas_h), (*glow_color, 0))
        glow_mask = expanded.point(lambda x: glow_alpha if x > 50 else 0)
        glow_layer.putalpha(glow_mask)
        glow_img = Image.alpha_composite(glow_img, glow_layer)
    
    # ぼかし
    glow_img = glow_img.filter(ImageFilter.GaussianBlur(radius=1.5))
    
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


def main():
    # Windows標準カーソルファイル
    system_cursor = r"C:\Windows\Cursors\aero_arrow.cur"
    
    if not os.path.exists(system_cursor):
        print(f"System cursor not found: {system_cursor}")
        return
    
    print(f"Reading system cursor: {system_cursor}")
    
    try:
        cursor_img, hotspot_x, hotspot_y = read_cur_file(system_cursor)
        print(f"Cursor size: {cursor_img.size}, Hotspot: ({hotspot_x}, {hotspot_y})")
        
        # 元のカーソルを保存
        cursor_img.save(os.path.join(OUTPUT_DIR, "original_aero_arrow.png"))
        
        # 32x32にリサイズ
        cursor_32 = cursor_img.resize((32, 32), Image.Resampling.LANCZOS)
        
        # 青グロー
        blue = add_glow_to_cursor(cursor_32, (0, 150, 255), glow_size=3)
        blue.save(os.path.join(OUTPUT_DIR, "cursor_blue_aero.png"))
        png_to_cur(blue, os.path.join(OUTPUT_DIR, "resources", "cursor_en.cur"), 
                   hotspot_x=2, hotspot_y=2)
        
        # 赤グロー
        red = add_glow_to_cursor(cursor_32, (255, 50, 50), glow_size=3)
        red.save(os.path.join(OUTPUT_DIR, "cursor_red_aero.png"))
        png_to_cur(red, os.path.join(OUTPUT_DIR, "resources", "cursor_jp.cur"),
                   hotspot_x=2, hotspot_y=2)
        
        print("\nDone! Cursors created from Windows system cursor.")
        
    except Exception as e:
        print(f"Error: {e}")
        import traceback
        traceback.print_exc()


if __name__ == "__main__":
    main()
