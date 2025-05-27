# Chess Client

A high-quality chess game client that connects to the MPChessServer, providing an intuitive and visually appealing interface for playing chess online.

## Features

- Complete chess rules implementation (including en passant, castling, pawn promotion)
- Server-authoritative game validation
- Player authentication and persistent accounts
- Rating system with Elo rankings
- Matchmaking with skill-based pairing
- Game analysis with optional StockFish integration
- Move recommendations
- Comprehensive leaderboards
- Detailed game history and replay functionality
- Multiple time control options
- Bot opponents with adjustable difficulty

## Building the Client

### Prerequisites

- Qt 6.5.3 or later
- C++17 compatible compiler
- CMake 3.16 or later

### Build Instructions

1. Clone the repository:
git clone <git repo>
cd chess-client


2. Create a build directory:
mkdir build cd build


3. Configure with CMake:
cmake ..


4. Build the project:
cmake --build .


5. Run the client:
./MPChessClient


## Connecting to a Server

By default, the client attempts to connect to a server running on `localhost:5000`. To connect to a different server:

1. Go to File > Connect to Server...
2. Enter the server address and port in the format `host:port`
3. Click Connect

## User Interface

### Main Tabs

- **Home**: Access matchmaking and start new games
- **Play**: The main game interface where you play chess
- **Analysis**: View game history and analyze games
- **Profile**: View your player profile and statistics
- **Leaderboard**: See top players by rating, wins, and win percentage

### Game Interface

- Chess board with drag-and-drop move functionality
- Move history panel showing all moves in standard notation
- Captured pieces display
- Game timer showing remaining time for both players
- Game controls (resign, offer draw)
- Chat functionality

### Analysis Features

- Move evaluation graphs
- Move recommendations
- Mistake identification
- Critical moment analysis
- Optional StockFish integration for professional-level analysis

## Keyboard Shortcuts

- **Ctrl+N**: Connect to server
- **Ctrl+D**: Disconnect from server
- **Ctrl+,**: Open settings
- **Alt+F4**: Exit
- **F**: Flip board
- **A**: Toggle analysis panel
- **C**: Toggle chat panel

## Settings

The client offers various customization options:

- **Theme**: Light, Dark, or Custom
- **Board Theme**: Classic, Wood, Marble, Blue, Green, or Custom
- **Piece Theme**: Classic, Modern, Simple, Fancy, or Custom
- **Sound Effects**: Enable/disable and adjust volume
- **Background Music**: Enable/disable and adjust volume

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Acknowledgments

- Chess piece SVGs from [Wikimedia Commons](https://commons.wikimedia.org/wiki/Category:SVG_chess_pieces)
- Sound effects from [freesound.org](https://freesound.org/)