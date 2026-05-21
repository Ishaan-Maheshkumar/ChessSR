# ChessSR
Trying to improve chess hand crafted evaluation and neural network readability with symbolic regression.
The main part of this project is the new innovation of symbolic regression to find formulas for chess.
It uses mordern chess engine tactics, combined with fomulas to to evaluate board positions.
The python script, trys to find formulas to basically copy a large stockfish's evaluations messaured in centipawns(will change to sigmoid function later)
As of right now, the formula can reach an average of 280 centipawns, or 2.8 pawns of a value diffrence from stockfish, I am aiming for ~100.
PySR was used as the symbolic regrression module, altough I could have saved time from making functions in both python and C++, PySR, was highly user friendly.

In order to play, please download all files, and run C++ on it by compiling both .cpp files and running it. I am doing this because I dont think putting .exe's here is safe. I will link this as my project demo. 

Download windows for windows, and linux for linux, I have not tested anything on mac, if you are playing on mac (or other os), let me know if it works or not!

Default mode is set to normal, hand crafted evaluation, in order to change it to Symbolic, simply change evaluate function to use SymbolicEval instead of SimpleEval (I will make it more user friendly soon, sorry about that!)

Credit to a friend for allowing me to train on their server!

Let me know if you have any questions!!!
# Depth is super low as of now to begin with, if you want to see max preformance you must increase it. (To prevent any SUPER OLD devices to crash).
