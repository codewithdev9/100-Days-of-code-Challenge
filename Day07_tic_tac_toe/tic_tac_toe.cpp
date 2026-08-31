#include <iostream>
#include <limits>
using namespace std;

char board[3][3];

void initBoard() {
  int num = 1;
  for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++)
      board[i][j] =
          '0' + num++; // fills 1-9 initially so player knows cell numbers
}

void printBoard() {
  cout << "\n";
  for (int i = 0; i < 3; i++) {
    cout << " " << board[i][0] << " | " << board[i][1] << " | " << board[i][2]
         << " \n";
    if (i < 2)
      cout << "---|---|---\n";
  }
  cout << "\n";
}

bool checkWin(char player) {
  for (int i = 0; i < 3; i++) {
    if (board[i][0] == player && board[i][1] == player && board[i][2] == player)
      return true;
    if (board[0][i] == player && board[1][i] == player && board[2][i] == player)
      return true;
  }
  if (board[0][0] == player && board[1][1] == player && board[2][2] == player)
    return true;
  if (board[0][2] == player && board[1][1] == player && board[2][0] == player)
    return true;
  return false;
}

bool isBoardFull() {
  for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++)
      if (board[i][j] != 'X' && board[i][j] != 'O')
        return false;
  return true;
}

int main() {
  initBoard();
  char currentPlayer = 'X';
  int move;

  cout << "===== TIC TAC TOE =====\n";
  cout << "Players ke liye cell number (1-9) enter karo:\n";
  printBoard();

  while (true) {
    cout << "Player " << currentPlayer << ", apni move choose karo (1-9): ";
    cin >> move;

    if (cin.fail()) {
      cin.clear();
      cin.ignore(numeric_limits<streamsize>::max(), '\n');
      cout << "Invalid input! Kripya ek valid number (1-9) enter karo.\n";
      continue;
    }

    if (move < 1 || move > 9) {
      cout << "Invalid input! 1 se 9 ke beech number daalo.\n";
      continue;
    }

    int row = (move - 1) / 3;
    int col = (move - 1) % 3;

    if (board[row][col] == 'X' || board[row][col] == 'O') {
      cout << "Ye cell already filled hai! Dusra number try karo.\n";
      continue;
    }

    board[row][col] = currentPlayer;
    printBoard();

    if (checkWin(currentPlayer)) {
      cout << "🎉 Player " << currentPlayer << " jeet gaya! 🎉\n";
      break;
    }

    if (isBoardFull()) {
      cout << "Match Draw ho gaya!\n";
      break;
    }

    currentPlayer = (currentPlayer == 'X') ? 'O' : 'X';
  }

  cout << "Game Over. Dhanyavaad khelne ke liye!\n";
  return 0;
}