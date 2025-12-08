"""
アプリケーションアイコン(.ico)を生成
"""
from PIL import Image, ImageDraw
import struct
import os

def create_app_icon(output_path: str):
    """シンプルなアプリアイコンを生成"""
    sizes = [16, 32, 48]
    images = []
    
    for size in sizes:
        img = Image.new('RGBA', (size, size), (0, 0, 0, 0))
        draw = ImageDraw.Draw(img)
        
        # 円形アイコン（半分青、半分赤）
        center = size // 2
        radius = size // 2 - 2
        
        # 左半分 青
        draw.pieslice(
            [center - radius, center - radius, center + radius, center + radius],
            start=90, end=270,
            fill=(0, 120, 255, 255)
        )
        
        # 右半分 赤
        draw.pieslice(
            [center - radius, center - radius, center + radius, center + radius],
            start=270, end=90,
            fill=(255, 60, 60, 255)
        )
        
        # 境界線
        draw.ellipse(
            [center - radius, center - radius, center + radius, center + radius],
            outline=(255, 255, 255, 200),
            width=1
        )
        
        images.append(img)
    
    # ICOファイルとして保存
    images[0].save(
        output_path,
        format='ICO',
        sizes=[(s, s) for s in sizes],
        append_images=images[1:]
    )
    print(f"Generated: {output_path}")


if __name__ == "__main__":
    base_dir = os.path.dirname(os.path.abspath(__file__))
    create_app_icon(os.path.join(base_dir, "resources", "app.ico"))
