#!/usr/bin/env python3
import os
import json
import requests
import shutil
from pathlib import Path

# Configuration
PIECE_SETS = {
    'classic': 'cburnett',    # Original clean vector style
    'modern': 'alpha',        # Modern minimalist style
    'minimalist': 'merida'    # Traditional tournament style
}

SOUND_EFFECTS = {
    'move.wav': 'https://freesound.org/people/mh2o/sounds/351518/download/351518__mh2o__chess-move-on-alabaster.wav',
    'capture.wav': 'https://freesound.org/people/InspectorJ/sounds/416179/download/416179__inspectorj__dropping-wood-light-a.wav',
    'check.wav': 'https://freesound.org/people/plasterbrain/sounds/237422/download/237422__plasterbrain__game-obstacle.wav',
    'checkmate.wav': 'https://freesound.org/people/jobro/sounds/60444/download/60444__jobro__dramatic-piano-2.wav',
    'start.wav': 'https://freesound.org/people/InspectorJ/sounds/411460/download/411460__inspectorj__dropping-wood-medium-a.wav',
    'draw_offer.wav': 'https://freesound.org/people/InspectorJ/sounds/411168/download/411168__inspectorj__dropping-wood-light-b.wav',
    'timeout.wav': 'https://freesound.org/people/InspectorJ/sounds/403014/download/403014__inspectorj__dropping-wood-heavy-a.wav',
    'resign.wav': 'https://freesound.org/people/InspectorJ/sounds/411167/download/411167__inspectorj__dropping-wood-heavy-b.wav'
}

THEME_DEFINITIONS = {
    'classic.json': {
        "name": "Classic",
        "board": {
            "lightSquares": "#FFFFFF",
            "darkSquares": "#769656",
            "border": "#404040",
            "highlightMove": "#BACA2B",
            "highlightCheck": "#FF0000"
        },
        "pieces": {
            "set": "classic",
            "scale": 0.9
        }
    },
    'modern.json': {
        "name": "Modern",
        "board": {
            "lightSquares": "#EEEED2",
            "darkSquares": "#769656",
            "border": "#404040",
            "highlightMove": "#829769",
            "highlightCheck": "#C84147"
        },
        "pieces": {
            "set": "modern",
            "scale": 0.85
        }
    },
    'minimalist.json': {
        "name": "Minimalist",
        "board": {
            "lightSquares": "#DEE3E6",
            "darkSquares": "#8CA2AD",
            "border": "#404040",
            "highlightMove": "#90A4AE",
            "highlightCheck": "#F44336"
        },
        "pieces": {
            "set": "minimalist",
            "scale": 0.9
        }
    }
}

# Updated Lichess piece URL template
LICHESS_PIECE_URL = "https://raw.githubusercontent.com/lichess-org/lila/master/public/piece/{set}/{piece}.svg"

def create_directory_structure():
    """Create the required directory structure."""
    base_dir = Path("assets")
    for dir_name in ['pieces', 'sounds', 'themes']:
        (base_dir / dir_name).mkdir(parents=True, exist_ok=True)
        if dir_name == 'pieces':
            for theme in PIECE_SETS.keys():
                (base_dir / dir_name / theme).mkdir(exist_ok=True)

def download_file(url, destination):
    """Download a file from URL to destination."""
    try:
        response = requests.get(url, stream=True)
        response.raise_for_status()
        with open(destination, 'wb') as f:
            shutil.copyfileobj(response.raw, f)
        print(f"Downloaded: {destination}")
        return True
    except Exception as e:
        print(f"Error downloading {url}: {e}")
        return False

def download_piece_sets():
    """Download chess piece sets from Lichess."""
    base_dir = Path("assets/pieces")
    
    # Piece names mapping (local_name: lichess_name)
    piece_names = {
        'white_king': 'wK',
        'white_queen': 'wQ',
        'white_rook': 'wR',
        'white_bishop': 'wB',
        'white_knight': 'wN',
        'white_pawn': 'wP',
        'black_king': 'bK',
        'black_queen': 'bQ',
        'black_rook': 'bR',
        'black_bishop': 'bB',
        'black_knight': 'bN',
        'black_pawn': 'bP'
    }
    
    for theme_name, set_name in PIECE_SETS.items():
        theme_dir = base_dir / theme_name
        print(f"\nDownloading {theme_name} piece set...")
        
        for local_name, piece_code in piece_names.items():
            url = LICHESS_PIECE_URL.format(set=set_name, piece=piece_code)
            destination = theme_dir / f"{local_name}.svg"
            success = download_file(url, destination)
            if not success:
                print(f"Failed to download: {url}")
            else:
                print(f"Successfully downloaded: {local_name} for {theme_name}")

def download_sounds():
    """Download sound effects."""
    sounds_dir = Path("assets/sounds")
    print("\nDownloading sound effects...")
    
    for sound_name, url in SOUND_EFFECTS.items():
        destination = sounds_dir / sound_name
        download_file(url, destination)

def create_theme_files():
    """Create theme JSON files."""
    themes_dir = Path("assets/themes")
    print("\nCreating theme files...")
    
    for theme_name, theme_data in THEME_DEFINITIONS.items():
        theme_file = themes_dir / theme_name
        with open(theme_file, 'w', encoding='utf-8') as f:
            json.dump(theme_data, f, indent=4)
        print(f"Created theme file: {theme_file}")

def create_resource_files():
    """Create Qt resource files."""
    print("\nCreating Qt resource files...")
    
    # Create pieces.qrc
    pieces_qrc = """<!DOCTYPE RCC>
<RCC version="1.0">
    <qresource prefix="/pieces">
"""
    for theme in PIECE_SETS.keys():
        for piece in ['king', 'queen', 'rook', 'bishop', 'knight', 'pawn']:
            for color in ['white', 'black']:
                pieces_qrc += f'        <file>pieces/{theme}/{color}_{piece}.svg</file>\n'
    pieces_qrc += """    </qresource>
</RCC>"""

    # Create sounds.qrc
    sounds_qrc = """<!DOCTYPE RCC>
<RCC version="1.0">
    <qresource prefix="/sounds">
"""
    for sound in SOUND_EFFECTS.keys():
        sounds_qrc += f'        <file>sounds/{sound}</file>\n'
    sounds_qrc += """    </qresource>
</RCC>"""

    # Create themes.qrc
    themes_qrc = """<!DOCTYPE RCC>
<RCC version="1.0">
    <qresource prefix="/themes">
"""
    for theme in THEME_DEFINITIONS.keys():
        themes_qrc += f'        <file>themes/{theme}</file>\n'
    themes_qrc += """    </qresource>
</RCC>"""

    # Write resource files
    with open('assets/pieces.qrc', 'w') as f:
        f.write(pieces_qrc)
    with open('assets/sounds.qrc', 'w') as f:
        f.write(sounds_qrc)
    with open('assets/themes.qrc', 'w') as f:
        f.write(themes_qrc)
    
    print("Created all Qt resource files")

def verify_downloads():
    """Verify all required files were downloaded successfully."""
    print("\nVerifying downloads...")
    missing_files = []
    
    # Check pieces
    for theme in PIECE_SETS.keys():
        for piece in ['king', 'queen', 'rook', 'bishop', 'knight', 'pawn']:
            for color in ['white', 'black']:
                piece_file = Path(f"assets/pieces/{theme}/{color}_{piece}.svg")
                if not piece_file.exists():
                    missing_files.append(str(piece_file))
    
    # Check sounds
    for sound in SOUND_EFFECTS.keys():
        sound_file = Path(f"assets/sounds/{sound}")
        if not sound_file.exists():
            missing_files.append(str(sound_file))
    
    # Check themes
    for theme in THEME_DEFINITIONS.keys():
        theme_file = Path(f"assets/themes/{theme}")
        if not theme_file.exists():
            missing_files.append(str(theme_file))
    
    if missing_files:
        print("\nWarning: The following files are missing:")
        for file in missing_files:
            print(f"  - {file}")
    else:
        print("All files downloaded successfully!")

def main():
    """Main function to download and organize all assets."""
    print("Starting chess assets download...")
    
    # Create directory structure
    create_directory_structure()
    
    # Download assets
    download_piece_sets()
    download_sounds()
    
    # Create theme files
    create_theme_files()
    
    # Create Qt resource files
    create_resource_files()
    
    # Verify downloads
    verify_downloads()
    
    print("\nAsset download complete!")

if __name__ == "__main__":
    main()