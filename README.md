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


***Depth is super low as of now to begin with, if you want to see max preformance you must increase it. (To prevent any SUPER OLD devices to crash).***




#  HOW IT WORKS
Chess engines use many algorithems inorder to efficently be able to play chess.
I will start of by explaining move generation.

Move generation in chess is mostly thought of as a 2d array with numbers representing pieces, atleast from what people around me thought. The real way too achieve blazing speeds is too use bitwise operators on integers. All integers are represented in binary on computers, binary is just 0s and 1s. Bitwise operators are kind of like ands, ors, nots, xors, xnors, etc. They are even called the same with just bitwise infront of it! Bit wise operators, takes to pieces of binary and treats each 1, as true, and 0 as false. And it compares 2 of theese integers, at the same index, and returns a new piece of binary with each 0 and 1 for every index, with the operator applied. So why do we use this in chess?? Bitwise operators are incredibly fast, and can be preformed millions of times per second on mordern cpus.

Now how can we use this in chess? Many langauages have data types for unsinged(no negative value) and specific amount of bits integers. For example, in C++, declaring a type "uint64_t" tells the computer that it is an unsigned 64 bit long integer. Chess engines use this because a chess board, is 8x8, or 64 squares. Each square can be represented by a 0 or 1 in a specific spot. Another major bitwise operator, is the left shift and right shift. ("<<" ">>"). If you habe any C++ knowledge you may know these as the cout, and cin symbols. What these do is they take a value on the left, and shift it right or left depending on the operator, by n units. for example 1<<n shifts the number 1, n units left.(Note in 1ULL is used in place of 1, to keep the unsigned long long type, or uint64_t). This method now prevents us from knowing what piece is on what square, as we only know if it is occupied or not, as it is 1/0. So we create a "board" for every piece type and color, and update every time. This may be inefficent, but it is much much faster than any 2d array can ever be because bitwise is incredibly fast.

How can this now be used in order to generate moves? Well pawns, kings, and knights are easy. We simply take all posible king moves by generating a ring around it and checking if piece can be captured or if it blocks. knights are simmilar as well. Tables can be used to precompute these patterns. Pawns are a bit more complicated due too the rules, but same proccess.

Queens, Rooks, and Bishops. These 3 pieces all slide, so we must check each piece one by one and each square one by one, or so you thought. A well developed trick for these pieces is called Magic Bitboards, first we make a function like I stated that uses loops to generate legal moves slow, and we loop through all 64 squares, trying to find a magic number to multiply relavent blockers and shift it, to essentially find the exaact position, a hashing algorithem. only rooks and bishops are needed, queen moves can be calculated by using the | operator to combine both.

Now comes check detection, we do this by generating all posible squares a side can attack and simply checking if king lands here.

In order to generate legal moves, we generate psuedo legal moves, and check if it is legal, the way used in my code, is technically slow and not the fastest because it has to do this every node.

Move ordering, is ordering moves based on checks, captures and etc. Ordering moves allows us to save search time because it lets us prune out unnecesary moves, you can search Alpha Beta search to get nice overviews.

And search. what search does is it goes deep into a game, if we just check 1 move ahead, we wont know what the oponet could do, the oopponent could be seeing 2 moves ahead, searching deeper allows us to pick the best move based on how good a position is, if we make this sequence of moves.

Quiscience search, is a search beyond normal search, when normal search ends, if there is any captures, it checks all depths until ano captures exist, this is used inorder to prevent the horizon effect, for example, if in 10 moves, if the engine captures a pawn, with a queen, it will be thinking that it has a leading edge, but it wont see that i can just capture the queen, unless we add this or simmilar.

Many more engine optimizations were made, but they are a bit more complicated, I would be glad to tell you more about if needed.

# The Symbolic Regrrssion

Symbolic Regrression is the proccess of finding and merging formulas, with values passed on, in order too create a formula that can predict a dataset. Simmilar to a neural network, where it can be trained to find a certain value, but neural networks run on matrix multiplaction and partial derivitives to train, we can not look at one and say, "This makes so much sense!!". This is where Symbolic Regression excels. We create a formulas based on a dataset, but we need paramaters for the function, so I created 2 copys of functions one in python(to train) and in C++(to play), theese "helper functions" are code functions, not math, they extract key values, like mobility of a board, and pass those as the parameters, instead of raw bitboards, which are erratic. PySR, now generates many formulas, and finds the best one. Altough Symbolic Regression to be used in chess, is a low probability now, formulas can be well under stood, and can be used to improve neural networks, not just in chess, but in CNN's etc, wherever pattern recognition excels. An example formula, in the begining of training:  

(int)(((((((material_balance() + 48.968533) / (phase() / 0.29228073)) + (((pawn_psqt_diff() + ((knight_psqt_diff() + bishop_psqt_diff()) / 0.61293954)) / phase()) + material_balance())) - (turn * -355.7223)) + pawn_advancement_diff())+simpleEval())/2);


This kind of program reveals what stockfish thinks, with enoug training, it is possible to replicate stockfish evals to 90+% and be able to understand how it plays because of the formula.
