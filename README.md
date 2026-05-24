# Chess Engine Dataset Evaluation via Symbolic Regression

An open source chess engine, that uses **Symbolic Regression** to reverse engineer and aproximate Stockfish's evaluation, in order to find gaps in the dataset.

**Read the full papers for in depth analysis**

## Key features
* **Custom Engine:** My own engine that uses low level optimizations for blazingly fast move generation and evaluation
* **Condensed Stockfish evaluation:** Uses formulas to aproximate the Neural Network of Stockfish
* **Symbolic Regression Script for Chess:** Generates formulas using traits of a board, found in mordern hand crafted evaluation engines.

## Languages used
* **Python with PySR:** Used to generate Symbolic formulas for chess boards
* **C++** Used for the true chess engine, do to its speed, and ability to optimize.

## Demo
### Windows:
 ```bash
git clone https://github.com/Ishaan-Maheshkumar/ChessSR.git
cd ChessSR
g++ WindowsFinalChess.cpp magics.cpp -O2 -std=c++17 -o chesssr
./chesssr
```
### Linux:
```bash
git clone https://github.com/Ishaan-Maheshkumar/ChessSR.git
cd ChessSR
g++ LinuxFinalChess.cpp magics.cpp -O2 -std=c++17 -o chesssr
./chesssr
```
