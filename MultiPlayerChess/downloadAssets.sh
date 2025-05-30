#!/bin/bash

# Create base directory structure
mkdir -p ./client/resources/icons
mkdir -p ./client/resources/pieces/classic
mkdir -p ./client/resources/pieces/modern
mkdir -p ./client/resources/sounds

echo "Downloading icons..."
# App icon
curl -L -o "./client/resources/icons/app_icon.svg" "https://raw.githubusercontent.com/feathericons/feather/master/icons/grid.svg"
curl -L -o "./client/resources/icons/app_icon.png" "https://raw.githubusercontent.com/feathericons/feather/master/icons/grid.svg"

# Flip board
curl -L -o "./client/resources/icons/flip_board.svg" "https://raw.githubusercontent.com/feathericons/feather/master/icons/refresh-cw.svg"
curl -L -o "./client/resources/icons/flip_board.png" "https://raw.githubusercontent.com/feathericons/feather/master/icons/refresh-cw.svg"

# Settings
curl -L -o "./client/resources/icons/settings.svg" "https://raw.githubusercontent.com/feathericons/feather/master/icons/settings.svg"
curl -L -o "./client/resources/icons/settings.png" "https://raw.githubusercontent.com/feathericons/feather/master/icons/settings.svg"

# Resign
curl -L -o "./client/resources/icons/resign.svg" "https://raw.githubusercontent.com/feathericons/feather/master/icons/flag.svg"
curl -L -o "./client/resources/icons/resign.png" "https://raw.githubusercontent.com/feathericons/feather/master/icons/flag.svg"

# Draw
curl -L -o "./client/resources/icons/draw.svg" "https://raw.githubusercontent.com/feathericons/feather/master/icons/user-check.svg"
curl -L -o "./client/resources/icons/draw.png" "https://raw.githubusercontent.com/feathericons/feather/master/icons/user-check.svg"

echo "Downloading classic chess pieces..."
# Classic pieces - white
curl -L -o "./client/resources/pieces/classic/white_pawn.svg" "https://upload.wikimedia.org/wikipedia/commons/4/45/Chess_plt45.svg"
curl -L -o "./client/resources/pieces/classic/white_knight.svg" "https://upload.wikimedia.org/wikipedia/commons/7/70/Chess_nlt45.svg"
curl -L -o "./client/resources/pieces/classic/white_bishop.svg" "https://upload.wikimedia.org/wikipedia/commons/b/b1/Chess_blt45.svg"
curl -L -o "./client/resources/pieces/classic/white_rook.svg" "https://upload.wikimedia.org/wikipedia/commons/7/72/Chess_rlt45.svg"
curl -L -o "./client/resources/pieces/classic/white_queen.svg" "https://upload.wikimedia.org/wikipedia/commons/1/15/Chess_qlt45.svg"
curl -L -o "./client/resources/pieces/classic/white_king.svg" "https://upload.wikimedia.org/wikipedia/commons/4/42/Chess_klt45.svg"

# Classic pieces - black
curl -L -o "./client/resources/pieces/classic/black_pawn.svg" "https://upload.wikimedia.org/wikipedia/commons/c/c7/Chess_pdt45.svg"
curl -L -o "./client/resources/pieces/classic/black_knight.svg" "https://upload.wikimedia.org/wikipedia/commons/e/ef/Chess_ndt45.svg"
curl -L -o "./client/resources/pieces/classic/black_bishop.svg" "https://upload.wikimedia.org/wikipedia/commons/9/98/Chess_bdt45.svg"
curl -L -o "./client/resources/pieces/classic/black_rook.svg" "https://upload.wikimedia.org/wikipedia/commons/f/ff/Chess_rdt45.svg"
curl -L -o "./client/resources/pieces/classic/black_queen.svg" "https://upload.wikimedia.org/wikipedia/commons/4/47/Chess_qdt45.svg"
curl -L -o "./client/resources/pieces/classic/black_king.svg" "https://upload.wikimedia.org/wikipedia/commons/f/f0/Chess_kdt45.svg"

# Classic pieces - white (PNG)
curl -L -o "./client/resources/pieces/classic/white_pawn.png" "https://upload.wikimedia.org/wikipedia/commons/4/45/Chess_plt45.svg"
curl -L -o "./client/resources/pieces/classic/white_knight.png" "https://upload.wikimedia.org/wikipedia/commons/7/70/Chess_nlt45.svg"
curl -L -o "./client/resources/pieces/classic/white_bishop.png" "https://upload.wikimedia.org/wikipedia/commons/b/b1/Chess_blt45.svg"
curl -L -o "./client/resources/pieces/classic/white_rook.png" "https://upload.wikimedia.org/wikipedia/commons/7/72/Chess_rlt45.svg"
curl -L -o "./client/resources/pieces/classic/white_queen.png" "https://upload.wikimedia.org/wikipedia/commons/1/15/Chess_qlt45.svg"
curl -L -o "./client/resources/pieces/classic/white_king.png" "https://upload.wikimedia.org/wikipedia/commons/4/42/Chess_klt45.svg"

# Classic pieces - black (PNG)
curl -L -o "./client/resources/pieces/classic/black_pawn.png" "https://upload.wikimedia.org/wikipedia/commons/c/c7/Chess_pdt45.svg"
curl -L -o "./client/resources/pieces/classic/black_knight.png" "https://upload.wikimedia.org/wikipedia/commons/e/ef/Chess_ndt45.svg"
curl -L -o "./client/resources/pieces/classic/black_bishop.png" "https://upload.wikimedia.org/wikipedia/commons/9/98/Chess_bdt45.svg"
curl -L -o "./client/resources/pieces/classic/black_rook.png" "https://upload.wikimedia.org/wikipedia/commons/f/ff/Chess_rdt45.svg"
curl -L -o "./client/resources/pieces/classic/black_queen.png" "https://upload.wikimedia.org/wikipedia/commons/4/47/Chess_qdt45.svg"
curl -L -o "./client/resources/pieces/classic/black_king.png" "https://upload.wikimedia.org/wikipedia/commons/f/f0/Chess_kdt45.svg"

echo "Downloading modern chess pieces..."
# Modern pieces - white
curl -L -o "./client/resources/pieces/modern/white_pawn.svg" "https://upload.wikimedia.org/wikipedia/commons/4/45/Chess_plt45.svg"
curl -L -o "./client/resources/pieces/modern/white_knight.svg" "https://upload.wikimedia.org/wikipedia/commons/7/70/Chess_nlt45.svg"
curl -L -o "./client/resources/pieces/modern/white_bishop.svg" "https://upload.wikimedia.org/wikipedia/commons/b/b1/Chess_blt45.svg"
curl -L -o "./client/resources/pieces/modern/white_rook.svg" "https://upload.wikimedia.org/wikipedia/commons/7/72/Chess_rlt45.svg"
curl -L -o "./client/resources/pieces/modern/white_queen.svg" "https://upload.wikimedia.org/wikipedia/commons/1/15/Chess_qlt45.svg"
curl -L -o "./client/resources/pieces/modern/white_king.svg" "https://upload.wikimedia.org/wikipedia/commons/4/42/Chess_klt45.svg"

# Modern pieces - black
curl -L -o "./client/resources/pieces/modern/black_pawn.svg" "https://upload.wikimedia.org/wikipedia/commons/c/c7/Chess_pdt45.svg"
curl -L -o "./client/resources/pieces/modern/black_knight.svg" "https://upload.wikimedia.org/wikipedia/commons/e/ef/Chess_ndt45.svg"
curl -L -o "./client/resources/pieces/modern/black_bishop.svg" "https://upload.wikimedia.org/wikipedia/commons/9/98/Chess_bdt45.svg"
curl -L -o "./client/resources/pieces/modern/black_rook.svg" "https://upload.wikimedia.org/wikipedia/commons/f/ff/Chess_rdt45.svg"
curl -L -o "./client/resources/pieces/modern/black_queen.svg" "https://upload.wikimedia.org/wikipedia/commons/4/47/Chess_qdt45.svg"
curl -L -o "./client/resources/pieces/modern/black_king.svg" "https://upload.wikimedia.org/wikipedia/commons/f/f0/Chess_kdt45.svg"

# Modern pieces - white (PNG)
curl -L -o "./client/resources/pieces/modern/white_pawn.png" "https://upload.wikimedia.org/wikipedia/commons/0/04/Chess_plt60.png"
curl -L -o "./client/resources/pieces/modern/white_knight.png" "https://upload.wikimedia.org/wikipedia/commons/2/28/Chess_nlt60.png"
curl -L -o "./client/resources/pieces/modern/white_bishop.png" "https://upload.wikimedia.org/wikipedia/commons/9/9b/Chess_blt60.png"
curl -L -o "./client/resources/pieces/modern/white_rook.png" "https://upload.wikimedia.org/wikipedia/commons/5/5c/Chess_rlt60.png"
curl -L -o "./client/resources/pieces/modern/white_queen.png" "https://upload.wikimedia.org/wikipedia/commons/4/49/Chess_qlt60.png"
curl -L -o "./client/resources/pieces/modern/white_king.png" "https://upload.wikimedia.org/wikipedia/commons/3/3b/Chess_klt60.png"

# Modern pieces - black (PNG)
curl -L -o "./client/resources/pieces/modern/black_pawn.png" "https://upload.wikimedia.org/wikipedia/commons/c/cd/Chess_pdt60.png"
curl -L -o "./client/resources/pieces/modern/black_knight.png" "https://upload.wikimedia.org/wikipedia/commons/f/f1/Chess_ndt60.png"
curl -L -o "./client/resources/pieces/modern/black_bishop.png" "https://upload.wikimedia.org/wikipedia/commons/8/81/Chess_bdt60.png"
curl -L -o "./client/resources/pieces/modern/black_rook.png" "https://upload.wikimedia.org/wikipedia/commons/a/a0/Chess_rdt60.png"
curl -L -o "./client/resources/pieces/modern/black_queen.png" "https://upload.wikimedia.org/wikipedia/commons/a/af/Chess_qdt60.png"
curl -L -o "./client/resources/pieces/modern/black_king.png" "https://upload.wikimedia.org/wikipedia/commons/e/e3/Chess_kdt60.png"

echo "Downloading sounds..."
# Sound files
curl -L -o "./client/resources/sounds/move.wav" "https://freesound.org/data/previews/240/240777_4107740-lq.mp3"
curl -L -o "./client/resources/sounds/capture.wav" "https://freesound.org/data/previews/240/240776_4107740-lq.mp3"
curl -L -o "./client/resources/sounds/check.wav" "https://freesound.org/data/previews/131/131660_2398403-lq.mp3"
curl -L -o "./client/resources/sounds/checkmate.wav" "https://freesound.org/data/previews/131/131662_2398403-lq.mp3"
curl -L -o "./client/resources/sounds/castle.wav" "https://freesound.org/data/previews/242/242857_4107740-lq.mp3"
curl -L -o "./client/resources/sounds/promotion.wav" "https://freesound.org/data/previews/270/270402_5123851-lq.mp3"
curl -L -o "./client/resources/sounds/game_start.wav" "https://freesound.org/data/previews/221/221683_1015240-lq.mp3"
curl -L -o "./client/resources/sounds/game_end.wav" "https://freesound.org/data/previews/277/277021_5267727-lq.mp3"
curl -L -o "./client/resources/sounds/error.wav" "https://freesound.org/data/previews/142/142608_1840739-lq.mp3"
curl -L -o "./client/resources/sounds/notification.wav" "https://freesound.org/data/previews/336/336998_4939433-lq.mp3"
curl -L -o "./client/resources/sounds/background_music.mp3" "https://freesound.org/data/previews/368/368388_6394861-lq.mp3"

echo "All resources have been downloaded!"