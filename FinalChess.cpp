# include<iostream>
#include <cstdint>
#include <vector>
#include <random>
#include<chrono>
#include "magics.h"
#include <cstring>
#include <string>
#include <thread>
#include <atomic>
#include <mutex>
using namespace std;

struct Move{
    uint8_t from, to, promotion;
    bool capture;
    bool special;
    int score;
};
struct Magic{
    int shift;
    uint64_t magic,mask;

};
struct Undo{
    uint64_t white_pawns;
    uint64_t white_knights;
    uint64_t white_bishops;
    uint64_t white_rooks;
    uint64_t white_queen;
    uint64_t white_king;
    uint64_t black_pawns;
    uint64_t black_knights;
    uint64_t black_bishops;
    uint64_t black_rooks;
    uint64_t black_queen;
    uint64_t black_king;
    uint64_t white_pieces;
    uint64_t black_pieces;
    uint64_t all_pieces;
    int enpass_target;
    int turn;
    int count;
    int castleRights;

};
struct TTentry{
    std::atomic<uint64_t> hash;
    std::atomic<int> eval;
    std::atomic<int> depth;
    std::atomic<int8_t> flag;
    Move bestMove;
    TTentry() : hash(0), eval(0), depth(0), flag(0) {}
    TTentry(const TTentry& o){
        hash.store(o.hash.load());
        eval.store(o.eval.load());
        depth.store(o.depth.load());
        flag.store(o.flag.load());
        bestMove = o.bestMove;
    }
    TTentry& operator=(const TTentry& o){
        hash.store(o.hash.load());
        eval.store(o.eval.load());
        depth.store(o.depth.load());
        flag.store(o.flag.load());
        bestMove = o.bestMove;
        return *this;
    }
};
class Board{

public:
    static const uint64_t RANK_1 = 0x00000000000000FF;
    static const uint64_t RANK_2 = 0x000000000000FF00;
    static const uint64_t RANK_3 = 0x0000000000FF0000;
    static const uint64_t RANK_4 = 0x00000000FF000000;
    static const uint64_t RANK_5 = 0x000000FF00000000;
    static const uint64_t RANK_6 = 0x0000FF0000000000;
    static const uint64_t RANK_7 = 0x00FF000000000000;
    static const uint64_t RANK_8 = 0xFF00000000000000;
    static const uint64_t FILE_A = 0x0101010101010101;
    static const uint64_t FILE_B = 0x0202020202020202;
    static const uint64_t FILE_C = 0x0404040404040404;
    static const uint64_t FILE_D = 0x0808080808080808;
    static const uint64_t FILE_E = 0x1010101010101010;
    static const uint64_t FILE_F = 0x2020202020202020;
    static const uint64_t FILE_G = 0x4040404040404040;
    static const uint64_t FILE_H = 0x8080808080808080;
   static constexpr int pawn_psqt[64] = {
     0,  0,  0,  0,  0,  0,  0,  0,
     5, 10, 10,-20,-20, 10, 10,  5,
     5, -5,-10,  0,  0,-10, -5,  5,
     0,  0,  0, 20, 20,  0,  0,  0,
     5,  5, 10, 25, 25, 10,  5,  5,
    10, 10, 20, 30, 30, 20, 10, 10,
    50, 50, 50, 50, 50, 50, 50, 50,
     0,  0,  0,  0,  0,  0,  0,  0
};

static constexpr int knight_psqt[64] = {
    -50, -40, -30, -30, -30, -30, -40, -50,
    -40, -20,   0,   0,   0,   0, -20, -40,
    -30,   0,  10,  15,  15,  10,   0, -30,
    -30,   5,  15,  20,  20,  15,   5, -30,
    -30,   0,  15,  20,  20,  15,   0, -30,
    -30,   5,  10,  15,  15,  10,   5, -30,
    -40, -20,   0,   5,   5,   0, -20, -40,
    -50, -40, -30, -30, -30, -30, -40, -50
};

static constexpr int bishop_psqt[64] = {
    -20, -10, -10, -10, -10, -10, -10, -20,
    -10,   0,   0,   0,   0,   0,   0, -10,
    -10,   0,   5,  10,  10,   5,   0, -10,
    -10,   5,   5,  10,  10,   5,   5, -10,
    -10,   0,  10,  10,  10,  10,   0, -10,
    -10,  10,  10,  10,  10,  10,  10, -10,
    -10,   5,   0,   0,   0,   0,   5, -10,
    -20, -10, -10, -10, -10, -10, -10, -20
};

static constexpr int rook_psqt[64] = {
      0,  0,  0,  0,  0,  0,  0,  0,
      5, 10, 10, 10, 10, 10, 10,  5,
     -5,  0,  0,  0,  0,  0,  0, -5,
     -5,  0,  0,  0,  0,  0,  0, -5,
     -5,  0,  0,  0,  0,  0,  0, -5,
     -5,  0,  0,  0,  0,  0,  0, -5,
     -5,  0,  0,  0,  0,  0,  0, -5,
      0,  0,  0,  5,  5,  0,  0,  0
};

static constexpr int queen_psqt[64] = {
    -20, -10, -10,  -5,  -5, -10, -10, -20,
    -10,   0,   0,   0,   0,   0,   0, -10,
    -10,   0,   5,   5,   5,   5,   0, -10,
     -5,   0,   5,   5,   5,   5,   0,  -5,
      0,   0,   5,   5,   5,   5,   0,  -5,
    -10,   5,   5,   5,   5,   5,   0, -10,
    -10,   0,   5,   0,   0,   0,   0, -10,
    -20, -10, -10,  -5,  -5, -10, -10, -20
};

static constexpr int king_psqt[64] = {
     20,  30,  10,   0,   0,  10,  30,  20,
     20,  20,   0,   0,   0,   0,  20,  20,
    -10, -20, -20, -20, -20, -20, -20, -10,
    -20, -30, -30, -40, -40, -30, -30, -20,
    -30, -40, -40, -50, -50, -40, -40, -30,
    -30, -40, -40, -50, -50, -40, -40, -30,
    -30, -40, -40, -50, -50, -40, -40, -30,
    -30, -40, -40, -50, -50, -40, -40, -30
};
static constexpr uint64_t FILE_MASKS[8] = {
    0x0101010101010101ULL,
    0x0202020202020202ULL,
    0x0404040404040404ULL,
    0x0808080808080808ULL,
    0x1010101010101010ULL,
    0x2020202020202020ULL,
    0x4040404040404040ULL,
    0x8080808080808080ULL
};


    uint64_t white_pawns;
    uint64_t white_knights;
    uint64_t white_bishops;
    uint64_t white_rooks;
    uint64_t white_queen;
    uint64_t white_king;
    uint64_t black_pawns;
    uint64_t black_knights;
    uint64_t black_bishops;
    uint64_t black_rooks;
    uint64_t black_queen;
    uint64_t black_king;
    uint64_t white_pieces;
    uint64_t black_pieces;
    uint64_t all_pieces;
    int enpass_target;
    int turn;
    int count;
    int legalCount;
    int captureCount;
    Move moves[256];
    Move legalMoves[256];
    Move captureMoves[256];

    uint64_t knightTable[64];
    uint64_t kingTable[64];
    Undo History[1024];
    Magic rookTable[64];
    Magic bishopTable[64];
    uint64_t pieceHash[64][12];
    uint64_t castleHash[16];
    uint64_t enpassHash[8];
    uint64_t blackToMove;
    uint64_t currentHash;
    uint64_t hashHistory[1024];
    static uint64_t rookAttackTable[64][1<<13];
    static uint64_t bishopAttackTable[64][1<<11];
    static TTentry tt[1<<22];
    
static std::atomic<bool> stopSearch;
static std::atomic<long long> nodeCount;
    int castleRights;

    int ply;
Board() {
    white_pawns   = 0x000000000000FF00;
    white_rooks   = 0x0000000000000081;
    white_knights = 0x0000000000000042;
    white_bishops = 0x0000000000000024;
    white_queen   = 0x0000000000000008;
    white_king    = 0x0000000000000010;

    black_pawns   = 0x00FF000000000000;
    black_rooks   = 0x8100000000000000;
    black_knights = 0x4200000000000000;
    black_bishops = 0x2400000000000000;
    black_queen   = 0x0800000000000000;
    black_king    = 0x1000000000000000;

    enpass_target = -1;
    turn = 0;
    count = 0;
    legalCount=0;
    captureCount=0;
    ply=0;
    castleRights=1|2|4|8;
    update_occupancy();
}
void update_occupancy(){
    white_pieces = (white_bishops|white_king|white_knights|white_pawns|white_queen|white_rooks);
    black_pieces=(black_bishops|black_king|black_knights|black_pawns|black_queen|black_rooks);
    all_pieces=(black_pieces|white_pieces);
}
inline int pop(uint64_t bb) {
    return __builtin_popcountll(bb);
}

inline int lsb(uint64_t bb) {
    return __builtin_ctzll(bb);
}

inline uint64_t pop_lsb(uint64_t &bb) {
    uint64_t b = bb & -bb;
    bb ^= b;
    return b;
}
int pawn_diff() { return pop(white_pawns) - pop(black_pawns); }
int knight_diff() { return pop(white_knights) - pop(black_knights); }
int bishop_diff() { return pop(white_bishops) - pop(black_bishops); }
int rook_diff() { return pop(white_rooks) - pop(black_rooks); }
int queen_diff() { return pop(white_queen) - pop(black_queen); }
int white_piece_count() { return pop(white_pieces); }
int black_piece_count() { return pop(black_pieces); }
int doubled_pawn_diff() {
    return doubled_pawns(white_pawns) - doubled_pawns(black_pawns);
}

int isolated_pawn_diff() {
    return isolated_pawns(white_pawns) - isolated_pawns(black_pawns);
}

int passed_pawn_diff() {
    return passed_pawns(white_pawns, black_pawns, true)
         - passed_pawns(black_pawns, white_pawns, false);
}

int pawn_advancement_diff() {
    return pawn_advancement(white_pawns, true)
         - pawn_advancement(black_pawns, false);
}
int king_shield_diff() {
    return king_shield(white_king, white_pawns, true)
         - king_shield(black_king, black_pawns, false);
}

int king_centrality_diff() {
    return king_centrality(white_king) - king_centrality(black_king);
}
int knight_mobility_diff() {
    return knight_mobility(white_knights, white_pieces)
         - knight_mobility(black_knights, black_pieces);
}

int rook_open_file_diff() {
    return rook_open_file(white_rooks, white_pawns, black_pawns)
         - rook_open_file(black_rooks, black_pawns, white_pawns);
}
int bishop_pair_diff() {
    return (pop(white_bishops) >= 2) - (pop(black_bishops) >= 2);
}
static constexpr uint64_t CENTRE =
    (1ULL<<27)|(1ULL<<28)|(1ULL<<35)|(1ULL<<36);

static constexpr uint64_t EXT_CENTRE =
    (1ULL<<18)|(1ULL<<19)|(1ULL<<20)|(1ULL<<21)|
    (1ULL<<26)|(1ULL<<29)|(1ULL<<34)|(1ULL<<37)|
    (1ULL<<42)|(1ULL<<43)|(1ULL<<44)|(1ULL<<45);

int centre_pawn_diff() {
    return pop(white_pawns & CENTRE) - pop(black_pawns & CENTRE);
}

int ext_centre_pawn_diff() {
    return pop(white_pawns & EXT_CENTRE) - pop(black_pawns & EXT_CENTRE);
}
int turn_feature() { return (turn == 0 ? 1 : -1); }
int white_can_castle() { return (castleRights & 3) ? 1 : 0; }
int black_can_castle() { return (castleRights & 12) ? 1 : 0; }
int white_material() {
    return 100*pop(white_pawns)
         + 320*pop(white_knights)
         + 330*pop(white_bishops)
         + 500*pop(white_rooks)
         + 900*pop(white_queen);
}

int black_material() {
    return 100*pop(black_pawns)
         + 320*pop(black_knights)
         + 330*pop(black_bishops)
         + 500*pop(black_rooks)
         + 900*pop(black_queen);
}

int material_balance() {
    return white_material() - black_material();
}

int total_material() {
    return white_material() + black_material();
}
int pawn_psqt_diff() {
    return psqt_sum(white_pawns, pawn_psqt, false)
         - psqt_sum(black_pawns, pawn_psqt, true);
}

int knight_psqt_diff() {
    return psqt_sum(white_knights, knight_psqt, false)
         - psqt_sum(black_knights, knight_psqt, true);
}

int bishop_psqt_diff() {
    return psqt_sum(white_bishops, bishop_psqt, false)
         - psqt_sum(black_bishops, bishop_psqt, true);
}

int rook_psqt_diff() {
    return psqt_sum(white_rooks, rook_psqt, false)
         - psqt_sum(black_rooks, rook_psqt, true);
}

int king_psqt_diff() {
    return psqt_sum(white_king, king_psqt, false)
         - psqt_sum(black_king, king_psqt, true);
}
int pawn_x_rook_diff() {
    return (pop(white_pawns) - pop(black_pawns)) *
           (pop(white_rooks) - pop(black_rooks));
}

int knight_x_queen_diff() {
    return (pop(white_knights) - pop(black_knights)) *
           (pop(white_queen) - pop(black_queen));
}

int bishop_x_rook_diff() {
    return (pop(white_bishops) - pop(black_bishops)) *
           (pop(white_rooks) - pop(black_rooks));
}

int pawn_sq_diff() {
    int d = pop(white_pawns) - pop(black_pawns);
    return d * d;
}

int rook_sq_diff() {
    int d = pop(white_rooks) - pop(black_rooks);
    return d * d;
}
float phase() {
    return float(total_material()) / 7800.0f;
}
float material_x_phase() {
    return material_balance() * phase();
}
float passed_pawn_x_endgame() {
    float eg = 1.0f - phase();
    return passed_pawn_diff() * eg;
}
float king_centrality_x_endgame() {
    float eg = 1.0f - phase();
    return king_centrality_diff() * eg;
}
int rook_seventh_diff() {
    return rook_seventh(white_rooks, true)
         - rook_seventh(black_rooks, false);
}
int pawn_seventh_diff() {
    uint64_t w7 = white_pawns & 0x00FF000000000000ULL;
    uint64_t b7 = black_pawns & 0x000000000000FF00ULL;
    return pop(w7) - pop(b7);
}
int knight_outpost_diff() {
    return knight_outpost(white_knights, black_pawns, true)
         - knight_outpost(black_knights, white_pawns, false);
}
int connected_rooks_diff() {
    return connected_rooks(white_rooks) - connected_rooks(black_rooks);
}
int development_diff() {
    return dev_score(white_knights, white_bishops, true)
         - dev_score(black_knights, black_bishops, false);
}
int doubled_pawns(uint64_t pawns) {
    int d = 0;
    for (int f = 0; f < 8; f++) {
        int c = pop(pawns & FILE_MASKS[f]);
        if (c > 1) d += c - 1;
    }
    return d;
}
int isolated_pawns(uint64_t pawns) {
    int iso = 0;
    for (int f = 0; f < 8; f++) {
        uint64_t fm = FILE_MASKS[f];
        if (!(pawns & fm)) continue;
        uint64_t adj = 0;
        if (f > 0) adj |= FILE_MASKS[f - 1];
        if (f < 7) adj |= FILE_MASKS[f + 1];
        if (!(pawns & adj)) iso++;
    }
    return iso;
}
int passed_pawns(uint64_t my_pawns, uint64_t their_pawns, bool white) {
    int cnt = 0;
    uint64_t bb = my_pawns;
    while (bb) {
        int sq = lsb(bb);
        bb &= bb - 1;
        int f = sq & 7;
        int r = sq >> 3;
        uint64_t front = 0;
        if (white) {
            for (int rr = r + 1; rr < 8; rr++) {
                if (f > 0) front |= 1ULL << (rr * 8 + f - 1);
                front |= 1ULL << (rr * 8 + f);
                if (f < 7) front |= 1ULL << (rr * 8 + f + 1);
            }
        } else {
            for (int rr = r - 1; rr >= 0; rr--) {
                if (f > 0) front |= 1ULL << (rr * 8 + f - 1);
                front |= 1ULL << (rr * 8 + f);
                if (f < 7) front |= 1ULL << (rr * 8 + f + 1);
            }
        }
        if (!(their_pawns & front)) cnt++;
    }
    return cnt;
}
int pawn_advancement(uint64_t pawns, bool white) {
    int total = 0;
    uint64_t bb = pawns;
    while (bb) {
        int sq = lsb(bb);
        bb &= bb - 1;
        int r = sq >> 3;
        total += white ? r : (7 - r);
    }
    return total;
}
int king_shield(uint64_t king, uint64_t pawns, bool white) {
    int sq = lsb(king);
    int f = sq & 7;
    int r = sq >> 3;
    int shield = 0;
    int rf = white ? r + 1 : r - 1;
    if (rf < 0 || rf > 7) return 0;
    for (int df = -1; df <= 1; df++) {
        int f2 = f + df;
        if (f2 < 0 || f2 > 7) continue;
        if (pawns & (1ULL << (rf * 8 + f2))) shield++;
    }
    return shield;
}
int king_centrality(uint64_t king) {
    int sq = lsb(king);
    int f = sq & 7;
    int r = sq >> 3;
    return -(abs(f - 3) + abs(r - 3));
}
int knight_mobility(uint64_t knights, uint64_t same) {
    int total = 0;
    uint64_t bb = knights;
    while (bb) {
        int sq = lsb(bb);
        bb &= bb - 1;
        total += pop(knightTable[sq] & ~same);
    }
    return total;
}
int rook_open_file(uint64_t rooks, uint64_t my_pawns, uint64_t their_pawns) {
    int cnt = 0;
    uint64_t bb = rooks;
    while (bb) {
        int sq = lsb(bb);
        bb &= bb - 1;
        uint64_t fm = FILE_MASKS[sq & 7];
        if (!(my_pawns & fm)) cnt++;
        if (!(their_pawns & fm)) cnt++;
    }
    return cnt;
}
int rook_seventh(uint64_t rooks, bool white) {
    uint64_t rank7 = white ? 0x00FF000000000000ULL : 0x000000000000FF00ULL;
    return pop(rooks & rank7);
}
int knight_outpost(uint64_t knights, uint64_t their_pawns, bool white) {
    uint64_t mask = white ? 0x0000FFFFFF000000ULL : 0x000000FFFFFF0000ULL;
    uint64_t bb = knights & mask;
    int cnt = 0;
    while (bb) {
        int sq = lsb(bb);
        bb &= bb - 1;
        int f = sq & 7;
        int r = sq >> 3;
        bool attacked = false;
        if (white) {
            if (f > 0 && (their_pawns & (1ULL << ((r - 1) * 8 + f - 1)))) attacked = true;
            if (f < 7 && (their_pawns & (1ULL << ((r - 1) * 8 + f + 1)))) attacked = true;
        } else {
            if (f > 0 && (their_pawns & (1ULL << ((r + 1) * 8 + f - 1)))) attacked = true;
            if (f < 7 && (their_pawns & (1ULL << ((r + 1) * 8 + f + 1)))) attacked = true;
        }
        if (!attacked) cnt++;
    }
    return cnt;
}
int connected_rooks(uint64_t rooks) {
    if (pop(rooks) < 2) return 0;
    int a = lsb(rooks);
    rooks &= rooks - 1;
    int b = lsb(rooks);
    return ((a >> 3) == (b >> 3) || (a & 7) == (b & 7)) ? 1 : 0;
}
int dev_score(uint64_t knights, uint64_t bishops, bool white) {
    uint64_t home = white ? 0x00000000000000FFULL : 0xFF00000000000000ULL;
    return pop((knights | bishops) & ~home);
}
int psqt_sum(uint64_t bb, const int table[64], bool flip) {
    int total = 0;
    while (bb) {
        int sq = lsb(bb);
        bb &= bb - 1;
        total += table[flip ? (sq ^ 56) : sq];
    }
    return total;
}
int symbolicEval(){
    return (int)(((((((material_balance() + 48.968533) / (phase() / 0.29228073)) + (((pawn_psqt_diff() + ((knight_psqt_diff() + bishop_psqt_diff()) / 0.61293954)) / phase()) + material_balance())) - (turn * -355.7223)) + pawn_advancement_diff())+simpleEval())/2);
}
int getPiece(int p)const{
    uint64_t s=1ULL<<p;
    if(s&white_pieces){
        if(s&white_pawns) return 1;
        else if(s&white_knights) return 2;
        else if(s&white_bishops) return 3;
        else if(s&white_rooks) return 4;
        else if(s&white_queen) return 5;
        else if(s&white_king) return 6;
    }
    else if(s&black_pieces){
        if(s&black_pawns) return -1;
        else if(s&black_knights) return -2;
        else if(s&black_bishops) return -3;
        else if(s&black_rooks) return -4;
        else if(s&black_queen) return -5;
        else if(s&black_king) return -6;

    }
    return 0;

}
bool occupied(int s){return (1ULL<<s)&all_pieces;}
bool White_occupied(int s){return (1ULL<<s)&white_pieces;}
bool Black_occupied(int s){return (1ULL<<s)&black_pieces;}
void addMove(Move moves[256],int& count,int from,int to,bool capture=false,int promotion=0,bool special=false){
    moves[count].from=from;
    moves[count].to=to;
    moves[count].capture=capture;
    moves[count].promotion=promotion;
    moves[count].special=special;
    count++;




}
void generatePawnMoves(Move moves[256],int&count){

    if(turn==0){
        uint64_t pawns=white_pawns;
        while(pawns){
        int src=__builtin_ctzll(pawns);
        uint64_t from=1ULL<<src;
        uint64_t oneStep=from<<8;
        pawns&=pawns-1;
        if(oneStep&& !(oneStep&all_pieces)){
            if(from&RANK_7){
            addMove(moves,count,src,src+8,false,1,true);
            addMove(moves,count,src,src+8,false,2,true);
            addMove(moves,count,src,src+8,false,3,true);
            addMove(moves,count,src,src+8,false,4,true);

            }
            else{

            addMove(moves,count,src,src+8,false,0,false);
            }

        }
        uint64_t twoStep=from<<16;
        if(from&RANK_2&&(twoStep&&!(twoStep&all_pieces)&&!(oneStep&all_pieces))){
            addMove(moves,count,src,src+16,false,0,true);
        }
        if(((from&~FILE_A)<<7)&black_pieces){
            if(from&RANK_7){
            addMove(moves,count,src,src+7,true,1,true);
            addMove(moves,count,src,src+7,true,2,true);
            addMove(moves,count,src,src+7,true,3,true);
            addMove(moves,count,src,src+7,true,4,true);

            }
            else{
            addMove(moves,count,src,src+7,true,0,false);
            }

        }
        if(((from&~FILE_H)<<9)&black_pieces){
            if(from&RANK_7){

            addMove(moves,count,src,src+9,true,1,true);
            addMove(moves,count,src,src+9,true,2,true);
            addMove(moves,count,src,src+9,true,3,true);
            addMove(moves,count,src,src+9,true,4,true);
            }
            else{
            addMove(moves,count,src,src+9,true,0,false);
            }
        }

        if((enpass_target!=-1)&&(from&RANK_5)){

            if(enpass_target==src+9){
                addMove(moves,count,src,src+9,true,0,true);

            }
            else if (enpass_target==src+7)
            {
                addMove(moves,count,src,src+7,true,0,true);

            }
            
        }

        }
    }
    else{
         uint64_t pawns=black_pawns;
        while(pawns){
        int src=__builtin_ctzll(pawns);
        uint64_t from=1ULL<<src;
        uint64_t oneStep=from>>8;
        pawns&=pawns-1;
        if(oneStep&& !(oneStep&all_pieces)){
            if(from&RANK_2){
            addMove(moves,count,src,src-8,false,1,true);
            addMove(moves,count,src,src-8,false,2,true);
            addMove(moves,count,src,src-8,false,3,true);
            addMove(moves,count,src,src-8,false,4,true);

            }
            else{

            addMove(moves,count,src,src-8,false,0,false);
            }

        }
        uint64_t twoStep=from>>16;
        if(from&RANK_7&&(twoStep&&!(twoStep&all_pieces)&&!(oneStep&all_pieces))){
            addMove(moves,count,src,src-16,false,0,true);
        }
        if(((from&~FILE_H)>>7)&white_pieces){
            if(from&RANK_2){
            addMove(moves,count,src,src-7,true,1,true);
            addMove(moves,count,src,src-7,true,2,true);
            addMove(moves,count,src,src-7,true,3,true);
            addMove(moves,count,src,src-7,true,4,true);

            }
            else{
            addMove(moves,count,src,src-7,true,0,false);
            }

        }
        if(((from&~FILE_A)>>9)&white_pieces){
            if(from&RANK_2){

            addMove(moves,count,src,src-9,true,1,true);
            addMove(moves,count,src,src-9,true,2,true);
            addMove(moves,count,src,src-9,true,3,true);
            addMove(moves,count,src,src-9,true,4,true);
            }
            else{
            addMove(moves,count,src,src-9,true,0,false);
            }
        }

        if((enpass_target!=-1)&&(from&RANK_4)){

            if(enpass_target==src-9){
                addMove(moves,count,src,src-9,true,0,true);

            }
            else if (enpass_target==src-7)
            {
                addMove(moves,count,src,src-7,true,0,true);

            }
            
        }

        }
    }

}
void generateKnightMoves(Move moves[256],int&count){


    //Might make it so it checks all posible knight moves at the same time, for speed.
    uint64_t knights;
    uint64_t opposite;
    uint64_t same;


    if(turn==0){knights=white_knights;opposite=black_pieces;same=white_pieces;}
    else {knights=black_knights;opposite=white_pieces;same=black_pieces;}
    while(knights){
        int src=__builtin_ctzll(knights);
        knights&=knights-1;
        uint64_t knightMoves=knightTable[src];
        while(knightMoves){
            int knightMoveSrc=__builtin_ctzll(knightMoves);
            uint64_t knightMove=1ULL<<knightMoveSrc;
            knightMoves&=knightMoves-1;
            if(knightMove&~same){
                if(knightMove&opposite){

                    addMove(moves,count,src,knightMoveSrc,true,0,true);
                }
                else{

                    addMove(moves,count,src,knightMoveSrc,false,0,false);

                }
            }

        }

    }


}
void generateKingMoves(){
    uint64_t king;
    uint64_t same;
    uint64_t oposite;

    if(turn==0){
        king=white_king;
        same=white_pieces;
        oposite=black_pieces;
    }
    else{
        king=black_king;
        same=black_pieces;
        oposite=white_pieces;       
    }
    int src=__builtin_ctzll(king);
    uint64_t kingMoves=kingTable[src];
    while(kingMoves){
        int kingMoveSrc=__builtin_ctzll(kingMoves);
        uint64_t kingMove=1ULL<<kingMoveSrc;
        kingMoves&=kingMoves-1;
        if(kingMove&~same){
            if(kingMove&oposite){
                addMove(moves,count,src,kingMoveSrc,true,0,true);
            }
            else{

                addMove(moves,count,src,kingMoveSrc,false,0,false);

            }

        }
    }


    

}
inline uint64_t safe(int s){

    return (s>=0&&s<64) ? 1ULL<<s:0ULL;
}
void initKnightMoves() {
    for (int square = 0; square < 64; square++) {
        uint64_t bb = 1ULL << square;
        uint64_t attacks = 0ULL;

        attacks |= (bb << 17) & ~FILE_A;
        attacks |= (bb << 15) & ~FILE_H;
        attacks |= (bb << 10) & ~(FILE_A | FILE_B);
        attacks |= (bb << 6)  & ~(FILE_G | FILE_H);

        attacks |= (bb >> 17) & ~FILE_H;
        attacks |= (bb >> 15) & ~FILE_A;
        attacks |= (bb >> 10) & ~(FILE_G | FILE_H);
        attacks |= (bb >> 6)  & ~(FILE_A | FILE_B);

        knightTable[square] = attacks;
    }
}
void initKingMoves(){
    for(int square=0;square<64;square++){
        kingTable[square]=
        (safe(square+8))|
        (safe(square-8))|
        (safe(square+7)&~FILE_H)|
        (safe(square+9)&~FILE_A)|
        (safe(square-7)&~FILE_A)|
        (safe(square-9)&~FILE_H)|
        (safe(square+1)&~FILE_H)|
        (safe(square-1)&~FILE_A);
    }
}
void initBoard(){
    ply=0;
    initKnightMoves();
    initKingMoves();
    initMagicNumsAndShifts();
    initAttackTables();
    initZobristTables();
    currentHash=calcZobristHash();
    hashHistory[0]=currentHash;
    initTT();
    generateLegals();

}
void generateRookMovesSlow(){
    uint64_t rooks;
    uint64_t oposite;
    uint64_t same;
    if(turn==0){
    rooks=white_rooks;
    oposite=black_pieces;
    same= white_pieces;
    }
    else{
    rooks=black_rooks;

    oposite=white_pieces;
    same= black_pieces;
    }
    while(rooks){
        int src=__builtin_ctzll(rooks);
        rooks&=rooks-1;
        int file=src%8;
        int rank=src/8;

        uint64_t rook=1ULL<<src;
      
       for(int r= rank+1; r<8; ++r){
        int to=r*8+file;
        uint64_t mask= 1ULL<<to;
        if (mask&same) break;
        if(mask&oposite){

            addMove(moves,count,src,to,true,0,true);
            break;
        }
        addMove(moves,count,src,to,false,0,false);

       }
        for(int r= rank-1; r>=0; --r){
        int to=r*8+file;
        uint64_t mask= 1ULL<<to;
        if (mask&same) break;
        if(mask&oposite){

            addMove(moves,count,src,to,true,0,true);
            break;
        }
        addMove(moves,count,src,to,false,0,false);

       }
        for(int f= file+1; f<8; ++f){
        int to=rank*8+f;
        uint64_t mask= 1ULL<<to;
        if (mask&same) break;
        if(mask&oposite){

            addMove(moves,count,src,to,true,0,true);
            break;
        }
        addMove(moves,count,src,to,false,0,false);

       }
    for(int f= file-1; f>=0; --f){
        int to=rank*8+f;
        uint64_t mask= 1ULL<<to;
        if (mask&same) break;
        if(mask&oposite){

            addMove(moves,count,src,to,true,0,true);
            break;
        }
        addMove(moves,count,src,to,false,0,false);

       }
    }
    




}
void generateBishopMovesSlow(){

    uint64_t bishops;
    uint64_t oposite;
    uint64_t same;
    if(turn==0){
    bishops=white_bishops;
    oposite=black_pieces;
    same= white_pieces;
    }
    else{
    bishops=black_bishops;

    oposite=white_pieces;
    same= black_pieces;
    }
    while(bishops){
        
        int src=__builtin_ctzll(bishops);
        bishops&=bishops-1;
        uint64_t bishop=1ULL<<src;
        for(int i=9;i<64;i+=9){
            uint64_t bishopMove=bishop<<i;
            if (!bishopMove) break;

            if(bishopMove&oposite){
                addMove(moves,count,src,src+i,true,0,true);
                break;

            }
            else if((bishopMove&same)||(bishopMove&FILE_H)){

                break;
            }
            else{

                addMove(moves,count,src,src+i,false,0,false);

            }
        }
  
        for(int i=9;i<64;i+=9){
            uint64_t bishopMove=bishop>>i;
            if (!bishopMove) break;
            if(bishopMove&oposite){
                addMove(moves,count,src,src-i,true,0,true);
                break;

            }
            else if((bishopMove&same)||(bishopMove&FILE_A)){

                break;
            }
            else{

                addMove(moves,count,src,src-i,false,0,false);

            }
        }

        for(int i=7;i<64;i+=7){
            uint64_t bishopMove=bishop<<i;
            if (!bishopMove) break;
            if(bishopMove&oposite){
                addMove(moves,count,src,src+i,true,0,true);
                break;

            }
            else if((bishopMove&same)||(bishopMove&FILE_A)){

                break;
            }
            else{

                addMove(moves,count,src,src+i,false,0,false);

            }
        }
 
        for(int i=7;i<64;i+=7){
            uint64_t bishopMove=bishop>>i;
            if (!bishopMove) break;
            if(bishopMove&oposite){
                addMove(moves,count,src,src-i,true,0,true);
                break;

            }
            else if((bishopMove&same)||(bishopMove&FILE_H)){

                break;
            }
            else{

                addMove(moves,count,src,src-i,false,0,false);

            }
        }
    }




}
void generateQueenMovesSlow(){
    uint64_t queens;
    uint64_t oposite;
    uint64_t same;
    if(turn==0){
    queens=white_queen;
    oposite=black_pieces;
    same= white_pieces;
    }
    else{
    queens=black_queen;

    oposite=white_pieces;
    same= black_pieces;
    }
    while(queens){
        int src=__builtin_ctzll(queens);
        queens&=queens-1;
        uint64_t queen=1ULL<<src;
        for(int i=8;i<64;i+=8){
            uint64_t queenMove=queen<<i;
            if (!queenMove) break;
            if(queenMove&oposite){
                addMove(moves,count,src,src+i,true,0,true);
                break;

            }
            else if((queenMove&same)){

                break;
            }
            else{

                addMove(moves,count,src,src+i,false,0,false);

            }
        }
        for(int i=8;i<64;i+=8){
            uint64_t queenMove=queen>>i;
            if (!queenMove) break;
            if(queenMove&oposite){
                addMove(moves,count,src,src-i,true,0,true);
                break;

            }
            else if((queenMove&same)){

                break;
            }
            else{

                addMove(moves,count,src,src-i,false,0,false);

            }
        }
        for(int i=1;i<8;i++){
            uint64_t queenMove=queen<<i;
            if (!queenMove) break;
            if(queenMove&oposite){
                addMove(moves,count,src,src+i,true,0,true);
                break;

            }
            else if((queenMove&same)||(queenMove&FILE_H)){

                break;
            }
            else{

                addMove(moves,count,src,src+i,false,0,false);

            }
        }
   for(int i=1;i<8;i++){
            uint64_t queenMove=queen>>i;
            if (!queenMove) break;
            if(queenMove&oposite){
                addMove(moves,count,src,src-i,true,0,true);
                break;

            }
            else if((queenMove&same)||(queenMove&FILE_A)){

                break;
            }
            else{

                addMove(moves,count,src,src-i,false,0,false);

            }
        }
         for(int i=9;i<64;i+=9){
            uint64_t queenMove=queen<<i;
            if (!queenMove) break;
            if(queenMove&oposite){
                addMove(moves,count,src,src+i,true,0,true);
                break;

            }
            else if((queenMove&same)||(queenMove&FILE_H)){

                break;
            }
            else{

                addMove(moves,count,src,src+i,false,0,false);

            }
        }
  
        for(int i=9;i<64;i+=9){
            uint64_t queenMove=queen>>i;
            if (!queenMove) break;
            if(queenMove&oposite){
                addMove(moves,count,src,src-i,true,0,true);
                break;

            }
            else if((queenMove&same)||(queenMove&FILE_A)){

                break;
            }
            else{

                addMove(moves,count,src,src-i,false,0,false);

            }
        }

        for(int i=7;i<64;i+=7){
            uint64_t queenMove=queen<<i;
            if (!queenMove) break;
            if(queenMove&oposite){
                addMove(moves,count,src,src+i,true,0,true);
                break;

            }
            else if((queenMove&same)||(queenMove&FILE_A)){

                break;
            }
            else{

                addMove(moves,count,src,src+i,false,0,false);

            }
        }
 
        for(int i=7;i<64;i+=7){
            uint64_t queenMove=queen>>i;
            if (!queenMove) break;
            if(queenMove&oposite){
                addMove(moves,count,src,src-i,true,0,true);
                break;

            }
            else if((queenMove&same)||(queenMove&FILE_H)){

                break;
            }
            else{

                addMove(moves,count,src,src-i,false,0,false);

            }
        }
    }




}
void generateAllMovesSlow(){
    count=0;
    generateBishopMovesSlow();
    generateKingMoves();
    generateKnightMoves(moves,count);
    generatePawnMoves(moves,count);
    generateQueenMovesSlow();
    generateRookMovesSlow();
}
int pieceToIndex(int piece){
    if(piece > 0) return piece - 1;
    else return piece * -1 + 5;
}

void makeMove(const Move& move){
    hashHistory[ply] = currentHash;

    History[ply].white_pawns    = white_pawns;
    History[ply].white_queen    = white_queen;
    History[ply].white_rooks    = white_rooks;
    History[ply].white_bishops  = white_bishops;
    History[ply].white_knights  = white_knights;
    History[ply].white_king     = white_king;
    History[ply].black_pawns    = black_pawns;
    History[ply].black_queen    = black_queen;
    History[ply].black_rooks    = black_rooks;
    History[ply].black_bishops  = black_bishops;
    History[ply].black_knights  = black_knights;
    History[ply].black_king     = black_king;
    History[ply].enpass_target  = enpass_target;
    History[ply].turn           = turn;
    History[ply].count          = count;
    History[ply].castleRights   = castleRights;

    int from      = move.from;
    int to        = move.to;
    bool capture  = move.capture;
    int piece     = getPiece(from);
    int promotion = move.promotion;

    currentHash ^= castleHash[castleRights];
    if(enpass_target != -1) currentHash ^= enpassHash[enpass_target % 8];
    if(turn == 1) currentHash ^= blackToMove;

    currentHash ^= pieceHash[from][pieceToIndex(piece)];

    if(capture){
        int cPiece = getPiece(to);
        if(cPiece != 0){
            currentHash ^= pieceHash[to][pieceToIndex(cPiece)];
            switch(cPiece){
                case  1: clearBit(white_pawns,   to); break;
                case  2: clearBit(white_knights, to); break;
                case  3: clearBit(white_bishops, to); break;
                case  4: clearBit(white_rooks,   to); break;
                case  5: clearBit(white_queen,   to); break;
                case -1: clearBit(black_pawns,   to); break;
                case -2: clearBit(black_knights, to); break;
                case -3: clearBit(black_bishops, to); break;
                case -4: clearBit(black_rooks,   to); break;
                case -5: clearBit(black_queen,   to); break;
            }
        }
    }

    switch(piece){
        case  1: clearBit(white_pawns,   from); setBit(white_pawns,   to); break;
        case  2: clearBit(white_knights, from); setBit(white_knights, to); break;
        case  3: clearBit(white_bishops, from); setBit(white_bishops, to); break;
        case  4: clearBit(white_rooks,   from); setBit(white_rooks,   to); break;
        case  5: clearBit(white_queen,   from); setBit(white_queen,   to); break;
        case  6: clearBit(white_king,    from); setBit(white_king,    to); break;
        case -1: clearBit(black_pawns,   from); setBit(black_pawns,   to); break;
        case -2: clearBit(black_knights, from); setBit(black_knights, to); break;
        case -3: clearBit(black_bishops, from); setBit(black_bishops, to); break;
        case -4: clearBit(black_rooks,   from); setBit(black_rooks,   to); break;
        case -5: clearBit(black_queen,   from); setBit(black_queen,   to); break;
        case -6: clearBit(black_king,    from); setBit(black_king,    to); break;
    }

    currentHash ^= pieceHash[to][pieceToIndex(piece)];

    if(promotion > 0 && (piece == 1 || piece == -1)){
        if(piece == 1){
            clearBit(white_pawns, to);
            currentHash ^= pieceHash[to][pieceToIndex(1)];
            if(promotion == 1){ setBit(white_queen,   to); currentHash ^= pieceHash[to][pieceToIndex(5)]; }
            else if(promotion == 2){ setBit(white_rooks,  to); currentHash ^= pieceHash[to][pieceToIndex(4)]; }
            else if(promotion == 3){ setBit(white_knights,to); currentHash ^= pieceHash[to][pieceToIndex(2)]; }
            else{                    setBit(white_bishops,to); currentHash ^= pieceHash[to][pieceToIndex(3)]; }
        } else {
            clearBit(black_pawns, to);
            currentHash ^= pieceHash[to][pieceToIndex(-1)];
            if(promotion == 1){ setBit(black_queen,   to); currentHash ^= pieceHash[to][pieceToIndex(-5)]; }
            else if(promotion == 2){ setBit(black_rooks,  to); currentHash ^= pieceHash[to][pieceToIndex(-4)]; }
            else if(promotion == 3){ setBit(black_knights,to); currentHash ^= pieceHash[to][pieceToIndex(-2)]; }
            else{                    setBit(black_bishops,to); currentHash ^= pieceHash[to][pieceToIndex(-3)]; }
        }
    }

    if(piece == 1 && from + 16 == to){
        enpass_target = from + 8;
    } else if(piece == -1 && from - 16 == to){
        enpass_target = from - 8;
    } else {
        enpass_target = -1;
    }

    if(move.special && move.capture && move.promotion == 0){
        if(piece == 1){
            currentHash ^= pieceHash[to - 8][pieceToIndex(-1)];
            clearBit(black_pawns, to - 8);
        } else if(piece == -1){
            currentHash ^= pieceHash[to + 8][pieceToIndex(1)];
            clearBit(white_pawns, to + 8);
        }
    }

    if(piece == 6 && from - to == 2){
        clearBit(white_rooks, 0); setBit(white_rooks, 3);
        currentHash ^= pieceHash[0][pieceToIndex(4)];
        currentHash ^= pieceHash[3][pieceToIndex(4)];
        castleRights &= ~3;
    } else if(piece == 6 && from - to == -2){
        clearBit(white_rooks, 7); setBit(white_rooks, 5);
        currentHash ^= pieceHash[7][pieceToIndex(4)];
        currentHash ^= pieceHash[5][pieceToIndex(4)];
        castleRights &= ~3;
    } else if(piece == -6 && from - to == 2){
        clearBit(black_rooks, 56); setBit(black_rooks, 59);
        currentHash ^= pieceHash[56][pieceToIndex(-4)];
        currentHash ^= pieceHash[59][pieceToIndex(-4)];
        castleRights &= ~12;
    } else if(piece == -6 && from - to == -2){
        clearBit(black_rooks, 63); setBit(black_rooks, 61);
        currentHash ^= pieceHash[63][pieceToIndex(-4)];
        currentHash ^= pieceHash[61][pieceToIndex(-4)];
        castleRights &= ~12;
    } else if(piece == 6){
        castleRights &= ~3;
    } else if(piece == -6){
        castleRights &= ~12;
    }

    if(piece == 4 && from == 0)  castleRights &= ~1;
    else if(piece == 4 && from == 7)  castleRights &= ~2;
    else if(piece == -4 && from == 56) castleRights &= ~4;
    else if(piece == -4 && from == 63) castleRights &= ~8;

    if(capture){
        if(to == 0)  castleRights &= ~1;
        if(to == 7)  castleRights &= ~2;
        if(to == 56) castleRights &= ~4;
        if(to == 63) castleRights &= ~8;
    }

    turn ^= 1;
    ply++;
    update_occupancy();

    currentHash ^= castleHash[castleRights];
    if(enpass_target != -1) currentHash ^= enpassHash[enpass_target % 8];
    currentHash ^= blackToMove;
}
void undoMove(){
    ply--;
    white_pawns   = History[ply].white_pawns;
    white_queen   = History[ply].white_queen;
    white_rooks   = History[ply].white_rooks;
    white_bishops = History[ply].white_bishops;
    white_knights = History[ply].white_knights;
    white_king    = History[ply].white_king;

    black_pawns   = History[ply].black_pawns;
    black_queen   = History[ply].black_queen;
    black_rooks   = History[ply].black_rooks;
    black_bishops = History[ply].black_bishops;
    black_knights = History[ply].black_knights;
    black_king    = History[ply].black_king;

    enpass_target = History[ply].enpass_target;
    turn          = History[ply].turn;
    count         = History[ply].count;
    castleRights         = History[ply].castleRights;
    currentHash=hashHistory[ply];
    update_occupancy();

}
void printBoard() {
    std::cout << "   a b c d e f g h" << std::endl;
    for (int i = 7; i >= 0; i--) {
        std::cout << i + 1 << " |";
        for (int j = 0; j < 8; j++) {
            int p = getPiece(i * 8 + j);
            
            if ((i + j) % 2 == 0) std::cout << "\033[48;5;237m"; 
            else std::cout << "\033[48;5;243m";

            if (p == 0) std::cout << "  ";
            else if (p ==  1) std::cout << "\u265F ";
            else if (p ==  2) std::cout << "\u265E ";
            else if (p ==  3) std::cout << "\u265D ";
            else if (p ==  4) std::cout << "\u265C ";
            else if (p ==  5) std::cout << "\u265B ";
            else if (p ==  6) std::cout << "\u265A ";
            else if (p == -1) std::cout << "\u2659 ";
            else if (p == -2) std::cout << "\u2658 ";
            else if (p == -3) std::cout << "\u2657 ";
            else if (p == -4) std::cout << "\u2656 ";
            else if (p == -5) std::cout << "\u2655 ";
            else if (p == -6) std::cout << "\u2654 ";

            std::cout << "\033[0m";
        }
        std::cout << "| " << i + 1 << "\n";
    }
    std::cout << "   a b c d e f g h" << std::endl << std::endl;
}



uint64_t generateRookMask(int sq){
    uint64_t mask=0ULL;
    int rank=sq/8;
    int file=sq%8;
    for(int r=rank+1;r<7;++r){
        mask|=(1ULL<<(r*8+file));

    }
    for(int f=file+1;f<7;++f){
        mask|=(1ULL<<(rank*8+f));

    }
    for(int r=rank-1;r>=1;--r){
        mask|=(1ULL<<(r*8+file));

    }
    for(int f=file-1;f>=1;--f){
        mask|=(1ULL<<(rank*8+f));

    }
    return mask;

}
uint64_t generateRookAttacks(int sq,uint64_t occ){
    uint64_t mask=0ULL;
    int rank=sq/8;
    int file=sq%8;
    for(int r=rank+1;r<8;++r){
        uint64_t move=(1ULL<<(r*8+file));
        mask|=move;
        if(move&occ){
            break;

        }

    }
   for(int r=rank-1;r>=0;--r){
        uint64_t move=(1ULL<<(r*8+file));
        mask|=move;
        if(move&occ){
            break;

        }

    }
for(int f=file+1;f<8;++f){
        uint64_t move=(1ULL<<(rank*8+f));
        mask|=move;
        if(move&occ){
            break;

        }

    }
    for(int f=file-1;f>=0;--f){
        uint64_t move=(1ULL<<(rank*8+f));
        mask|=move;
        if(move&occ){
            break;

        }

    }
    return mask;

}
uint64_t generateBishopMask(int sq){
    int file=sq%8;
    int rank=sq/8;
    uint64_t mask=0ULL;
    for(int r=rank+1, f=file+1;r<7&&f<7;r++,f++){

        mask|=(1ULL<<(r*8+f));
    }
for(int r=rank-1, f=file+1;r>=1&&f<7;r--,f++){

        mask|=(1ULL<<(r*8+f));
    }
    for(int r=rank+1, f=file-1;r<7&&f>=1;r++,f--){

        mask|=(1ULL<<(r*8+f));
    }
    for(int r=rank-1, f=file-1;r>=1&&f>=1;r--,f--){

        mask|=(1ULL<<(r*8+f));
    }
    return mask;
}
uint64_t generateBishopAttacks(int sq,uint64_t occ){
    int file=sq%8;
    int rank=sq/8;
    uint64_t mask=0ULL;
    for(int r=rank+1, f=file+1;r<=7&&f<=7;r++,f++){
            uint64_t move=(1ULL<<(r*8+f));
        mask|=move;
        if(move&occ){
            break;
        }
    }
for(int r=rank-1, f=file+1;r>=0&&f<=7;r--,f++){

            uint64_t move=(1ULL<<(r*8+f));
        mask|=move;
        if(move&occ){
            break;
        }    }
    for(int r=rank+1, f=file-1;r<=7&&f>=0;r++,f--){

            uint64_t move=(1ULL<<(r*8+f));
        mask|=move;
        if(move&occ){
            break;
        }    }
    for(int r=rank-1, f=file-1;r>=0&&f>=0;r--,f--){

            uint64_t move=(1ULL<<(r*8+f));
        mask|=move;
        if(move&occ){
            break;
        }    }
    return mask;
}
void printBB(uint64_t BB){
    for(int j=7;j>=0;j--){
    for(int i=0;i<8;i++){
        if(BB&(1ULL<<(j*8+i))){
            std::cout<<"\033[91m"<<"1 "<<"\033[0m";

        }

        else{
            std::cout<<"0 ";
        }
    }
    std::cout<<"\n";
}
std::cout<<endl;
}
uint64_t generateAttackMask(bool byBlack){
    uint64_t mask=0ULL;
    if(!byBlack){
        mask |= ((white_pawns & ~FILE_H) << 9);
mask |= ((white_pawns & ~FILE_A) << 7);
        uint64_t knights=white_knights;
        while(knights){
            mask|=knightTable[__builtin_ctzll(knights)];
            knights&=knights-1;

        }
        mask|=kingTable[__builtin_ctzll(white_king)]; 
        uint64_t rook=white_rooks;
        uint64_t bishop=white_bishops;
        uint64_t queen=white_queen;
        while(rook){
            int sq=__builtin_ctzll(rook);
            mask|=(rookAttackTable[sq][magicIndex(rookTable[sq].magic,all_pieces&rookTable[sq].mask,rookTable[sq].shift)]);
            rook&=rook-1;
        }
        while(bishop){
            int sq=__builtin_ctzll(bishop);
            mask|=(bishopAttackTable[sq][magicIndex(bishopTable[sq].magic,all_pieces&bishopTable[sq].mask,bishopTable[sq].shift)]);
            bishop&=bishop-1;
        }
        while(queen){
            int sq=__builtin_ctzll(queen);
            mask|=((rookAttackTable[sq][magicIndex(rookTable[sq].magic,all_pieces&rookTable[sq].mask,rookTable[sq].shift)])|(bishopAttackTable[sq][magicIndex(bishopTable[sq].magic,all_pieces&bishopTable[sq].mask,bishopTable[sq].shift)]));
            queen&=queen-1;
        }

        return mask;
    }
    
    
    else{
         mask |= ((black_pawns & ~FILE_H) >> 7);
mask |= ((black_pawns & ~FILE_A) >> 9);
        uint64_t knights=black_knights;
        while(knights){
            mask|=knightTable[__builtin_ctzll(knights)];
            knights&=knights-1;

        }
        mask|=kingTable[__builtin_ctzll(black_king)]; 
        uint64_t rook=black_rooks;
        uint64_t bishop=black_bishops;
        uint64_t queen=black_queen;
        while(rook){
            int sq=__builtin_ctzll(rook);
            mask|=(rookAttackTable[sq][magicIndex(rookTable[sq].magic,all_pieces&rookTable[sq].mask,rookTable[sq].shift)]);
            rook&=rook-1;
        }
        while(bishop){
            int sq=__builtin_ctzll(bishop);
            mask|=(bishopAttackTable[sq][magicIndex(bishopTable[sq].magic,all_pieces&bishopTable[sq].mask,bishopTable[sq].shift)]);
            bishop&=bishop-1;
        }
        while(queen){
            int sq=__builtin_ctzll(queen);
            mask|=((rookAttackTable[sq][magicIndex(rookTable[sq].magic,all_pieces&rookTable[sq].mask,rookTable[sq].shift)])|(bishopAttackTable[sq][magicIndex(bishopTable[sq].magic,all_pieces&bishopTable[sq].mask,bishopTable[sq].shift)]));
            queen&=queen-1;
        }
        return mask;
    }

    return 0ULL;
}
void initRookAttackTable(int sq){
    rookTable[sq].mask=generateRookMask(sq);
    //rookTable[sq].shift=64-__builtin_popcountll(rookTable[sq].mask);
    uint64_t subset=0;
    do{
        uint64_t index=magicIndex(rookTable[sq].magic,subset,rookTable[sq].shift);
        rookAttackTable[sq][index]=generateRookAttacks(sq,subset);
        subset=(subset-rookTable[sq].mask)&rookTable[sq].mask;
    }while(subset);

}
void initBishopAttackTable(int sq){
    bishopTable[sq].mask=generateBishopMask(sq);
    //bishopTable[sq].shift=64-__builtin_popcountll(bishopTable[sq].mask);
    uint64_t subset=0;
    do{
               uint64_t index=magicIndex(bishopTable[sq].magic,subset,bishopTable[sq].shift);
        bishopAttackTable[sq][index]=generateBishopAttacks(sq,subset);
        subset=(subset-bishopTable[sq].mask)&bishopTable[sq].mask;
    }while(subset);

}
void initAttackTables(){

    for(int i=0;i<64;i++){
initBishopAttackTable(i);
initRookAttackTable(i);

    }
}
inline int magicIndex(uint64_t magic, uint64_t blockers,int shift){
    return (int)((blockers*magic)>>shift);
}
inline void setBit(uint64_t& bb,int sq){bb|=(1ULL<<sq);}
inline void clearBit(uint64_t& bb,int sq){bb&=~(1ULL<<sq);}
bool initMagicRook(int sq,int bits){
    uint64_t mask=rookTable[sq].mask;
    int shift=64-bits;
    int tableSize=1<<bits;
    std::random_device rd;
    std::mt19937_64 gen(rd());

    std::uniform_int_distribution<uint64_t> dist;
    uint64_t tempTable[tableSize];
     bool used[tableSize];
    int attempts=0;

    
    for(;;){
        for(int i=0;i<tableSize;i++){
        tempTable[i]=0ULL;
        used[i]=false;
    }
        attempts++;
        if(attempts%100000==0){
                std::cout<<"Rook square: "<<sq<<"\nAttempts: "<<attempts<<endl;
            }
        uint64_t rand=dist(gen);
        uint64_t subset=0;
        int linearIndex=0;
        bool collide=false;
        do{ 
            uint64_t attack=rookAttackTable[sq][linearIndex];

            int index=magicIndex(rand,subset&mask,shift);
            
            if (!used[index]){
                tempTable[index]=attack;
                used[index]=true;
            }
            else if(tempTable[index]!=attack){

                collide=true;
                break;
            }
            subset=(subset-mask)&mask;
            linearIndex++;
        }while(subset);
        if(!collide){
            rookTable[sq].shift=shift;
            rookTable[sq].magic=rand;
            return true;

        }

    }
    return true;
}
bool initMagicBishop(int sq,int bits){
    uint64_t mask=bishopTable[sq].mask;
    int shift=64-bits;
    int tableSize=1<<bits;
     std::random_device rd;
    std::mt19937_64 gen(rd());

    std::uniform_int_distribution<uint64_t> dist;
    uint64_t tempTable[tableSize];
    bool used[tableSize];
   int attempts=0;
    for(;;){
         for(int i=0;i<tableSize;i++){
        tempTable[i]=0ULL;
        used[i]=false;
    }
    attempts++;
    if(attempts%100000==0){
                std::cout<<"Bishop square: "<<sq<<"\nAttempts: "<<attempts<<endl;
            }
        uint64_t rand=dist(gen);
        uint64_t subset=0;
        int linearIndex=0;
        bool collide=false;
        do{ 
            uint64_t attack=bishopAttackTable[sq][linearIndex];

            int index=magicIndex(rand,subset&mask,shift);

            if (!used[index]){
                tempTable[index]=attack;
                used[index]=true;
            }
            else if(tempTable[index]!=attack){

                collide=true;
                break;
            }
            subset=(subset-mask)&mask;
            linearIndex++;
        }while(subset);
        if(!collide){
            bishopTable[sq].shift=shift;
            bishopTable[sq].magic=rand;
            return true;

        }

    }
    return true;
}
void initMagics(int rookBits,int bishopBits){
    for(int i=0;i<64;i++){
        initMagicRook(i,rookBits);
        initMagicBishop(i,bishopBits);

    }
}
void initMagicNumsAndShifts(){
    for(int i=0;i<64;i++){
        rookTable[i].magic=rookMagics[i];
        rookTable[i].shift=64-ROOK_BITS;
        bishopTable[i].magic=bishopMagics[i];
        bishopTable[i].shift=64-BISHOP_BITS;
    }
}
void printMagics(){
    std::cout << "const uint64_t rookMagics[64] = {\n";
for (int sq = 0; sq < 64; ++sq) {
    std::cout << "    0x" << std::hex << std::uppercase
              <<rookTable[sq].magic << "ULL";
    if (sq < 63) std::cout << ",";
    std::cout << "\n";
}
std::cout << "};\n";

std::cout << "const uint64_t bishopMagics[64] = {\n";
for (int sq = 0; sq < 64; ++sq) {
    std::cout << "    0x" << std::hex << std::uppercase
              << bishopTable[sq].magic << "ULL";
    if (sq < 63) std::cout << ",";
    std::cout << "\n";
}
std::cout << "};\n";
}
void generateRookMoves(){
    uint64_t rooks;
    uint64_t same;
    uint64_t op;

    if(turn==0){
        rooks=white_rooks;
        same=white_pieces;
        op=black_pieces;
    }
    else{
        rooks=black_rooks;
        same=black_pieces;
        op=white_pieces;

    }
    while(rooks){
        int src=__builtin_ctzll(rooks);
        rooks&=rooks-1;
        uint64_t atk=rookAttackTable[src][magicIndex(rookTable[src].magic,all_pieces&rookTable[src].mask,rookTable[src].shift)];
        uint64_t realAtk=atk&~same;
        while(realAtk){
            int oneAtkSrc=__builtin_ctzll(realAtk);
            uint64_t oneAtk=1ULL<<oneAtkSrc;
            if(oneAtk&op){
                addMove(moves,count,src,oneAtkSrc,true,0,true);
            }
            else{
                addMove(moves,count,src,oneAtkSrc,false,0,false);
           
            }
            realAtk&=realAtk-1;
        }


    }
}
void generateBishopMoves(){
    uint64_t bishop;
    uint64_t same;
    uint64_t op;

    if(turn==0){
        bishop=white_bishops;
        same=white_pieces;
        op=black_pieces;
    }
    else{
        bishop=black_bishops;
        same=black_pieces;
        op=white_pieces;

    }
    while(bishop){
        int src=__builtin_ctzll(bishop);
        bishop&=bishop-1;
        uint64_t atk=bishopAttackTable[src][magicIndex(bishopTable[src].magic,all_pieces&bishopTable[src].mask,bishopTable[src].shift)];
        uint64_t realAtk=atk&~same;
        while(realAtk){
            int oneAtkSrc=__builtin_ctzll(realAtk);
            uint64_t oneAtk=1ULL<<oneAtkSrc;
            if(oneAtk&op){
                addMove(moves,count,src,oneAtkSrc,true,0,true);
            }
            else{
                addMove(moves,count,src,oneAtkSrc,false,0,false);
           
            }
            realAtk&=realAtk-1;
        }


    }
}
void generateQueenMoves(){
    uint64_t queen;
    uint64_t same;
    uint64_t op;

    if(turn==0){
        queen=white_queen;
        same=white_pieces;
        op=black_pieces;
    }
    else{
        queen=black_queen;
        same=black_pieces;
        op=white_pieces;

    }
    while(queen){
        int src=__builtin_ctzll(queen);
        queen&=queen-1;
        uint64_t atk=(bishopAttackTable[src][magicIndex(bishopTable[src].magic,all_pieces&bishopTable[src].mask,bishopTable[src].shift)])|(rookAttackTable[src][magicIndex(rookTable[src].magic,all_pieces&rookTable[src].mask,rookTable[src].shift)]);
        uint64_t realAtk=atk&~same;
        while(realAtk){
            int oneAtkSrc=__builtin_ctzll(realAtk);
            uint64_t oneAtk=1ULL<<oneAtkSrc;
            if(oneAtk&op){
                addMove(moves,count,src,oneAtkSrc,true,0,true);
            }
            else{
                addMove(moves,count,src,oneAtkSrc,false,0,false);
           
            }
            realAtk&=realAtk-1;
        }


    }
}
void generateAllMoves(){
    count=0;
    generateBishopMoves();
    generatePawnMoves(moves,count);
    generateKnightMoves(moves,count);
    generateKingMoves();
    generateRookMoves();
    generateQueenMoves();
    genCastleMoves();
}
bool isCheck(){
    if(turn==0){
        if (white_king&generateAttackMask(true)){
            return true;
        }
    }
    else{
        if (black_king&generateAttackMask(false)){
            return true;
        }
    }
    return false;
}
void fillLegalMoves(){
    int c = 0;
    int myTurn = turn;
    uint64_t attacked = generateAttackMask(myTurn == 0);
    for(int i = 0; i < count; i++){
        makeMove(moves[i]);
        bool inCheck = (myTurn==0)
            ? (white_king & generateAttackMask(true))
            : (black_king & generateAttackMask(false));
        if(!inCheck) legalMoves[c++] = moves[i];
        undoMove();
    }
    legalCount = c;
}
void generateLegals(){
    generateAllMoves();
    fillLegalMoves();
    generateCaptureMoves();
}
void generateCaptureMoves(){
    int c=0;
    for(int i=0;i<legalCount;i++){
        if(legalMoves[i].capture){
            captureMoves[c++]=legalMoves[i];
        }
        /*
        else{
            makeMove(legalMoves[i]);
            bool givesCheck=isCheck();
            undoMove();
            if(givesCheck){
                captureMoves[c++]=legalMoves[i];
            }
        }
        */
    }
    captureCount=c;
}
void printMoves(Move a [],int count){
    for(int i=0;i<count;i++){
        std::cout<<(int)a[i].from<<" >> "<<(int)a[i].to<<endl;
    }

}
int simpleEval(){
    int score=0;
    score+=100*__builtin_popcountll(white_pawns);
    score+=320*__builtin_popcountll(white_knights);
    score+=330*__builtin_popcountll(white_bishops);
    score+=500*__builtin_popcountll(white_rooks);
    score+=900*__builtin_popcountll(white_queen);

    score-=100*__builtin_popcountll(black_pawns);
    score-=320*__builtin_popcountll(black_knights);
    score-=330*__builtin_popcountll(black_bishops);
    score-=500*__builtin_popcountll(black_rooks);
    score-=900*__builtin_popcountll(black_queen);
   // score+=(__builtin_popcountll(generateAttackMask(false))-__builtin_popcountll(generateAttackMask(true)))*5;
    uint64_t wp=white_pawns;
    uint64_t wn=white_knights;
    uint64_t wk=white_king;
    uint64_t bp=black_pawns;
    uint64_t bn=black_knights;
    uint64_t bk=black_king;
    uint64_t bq=black_queen;
    uint64_t bb=black_bishops;
    uint64_t br=black_rooks;
    uint64_t wq=white_queen;
    uint64_t wb=white_bishops;
    uint64_t wr=white_rooks;

    while(wp){
        int src=__builtin_ctzll(wp);
        wp&=wp-1;
        score+=pawn_psqt[src];
    }
    while(wn){
        int src=__builtin_ctzll(wn);
        wn&=wn-1;
        score+=knight_psqt[src];
    }
    while(wk){
        int src=__builtin_ctzll(wk);
        wk&=wk-1;
        score+=king_psqt[src];
    }
    while(bp){
        int src=__builtin_ctzll(bp);
        bp&=bp-1;
        score-=pawn_psqt[src^56];
    }
    while(bn){
        int src=__builtin_ctzll(bn);
        bn&=bn-1;
        score-=knight_psqt[src^56];
    }
    while(bk){
        int src=__builtin_ctzll(bk);
        bk&=bk-1;
        score-=king_psqt[src^56];
    }


    while(wq){
        int src=__builtin_ctzll(wq);
        wq&=wq-1;
        score+=queen_psqt[src];
    }
    while(wb){
        int src=__builtin_ctzll(wb);
        wb&=wb-1;
        score+=bishop_psqt[src];
    }
    while(wr){
        int src=__builtin_ctzll(wr);
        wr&=wr-1;
        score+=rook_psqt[src];
    }
      while(bq){
        int src=__builtin_ctzll(bq);
        bq&=bq-1;
        score-=queen_psqt[src^56];
    }
    while(bb){
        int src=__builtin_ctzll(bb);
        bb&=bb-1;
        score-=bishop_psqt[src^56];
    }
    while(br){
        int src=__builtin_ctzll(br);
        br&=br-1;
        score-=rook_psqt[src^56];
    }
    return score;
}
int minimax(int alpha, int beta, int depth){
    if(isDrawByRep()) return 0;

    int originalAlpha = alpha;
    TTentry& entry = tt[currentHash & ((1<<22)-1)];
    uint64_t entryHash  = entry.hash.load();
    int entryEval       = entry.eval.load();
    int entryDepth      = entry.depth.load();
    int8_t entryFlag    = entry.flag.load();

    if(entryHash == currentHash && entryDepth >= depth){
        if(entryFlag == 0) return entryEval;
        if(entryFlag == 1) alpha = max(alpha, entryEval);
        if(entryFlag == 2) beta  = min(beta,  entryEval);
        if(alpha >= beta)  return entryEval;
    }

    if(depth==0) return quiesce(alpha,beta);

    Move savedLegal[256];
    int savedLegalCount = legalCount;
    for(int i=0;i<legalCount;i++) savedLegal[i]=legalMoves[i];

    generateLegals();
    int myLegalCount = legalCount;
    Move myLegal[256];
    for(int i=0;i<legalCount;i++) myLegal[i]=legalMoves[i];

    for(int i=0;i<savedLegalCount;i++) legalMoves[i]=savedLegal[i];
    legalCount=savedLegalCount;

    if(myLegalCount==0){
        if(isCheck()) return turn==0 ? -999999+ply : 999999-ply;
        else return 0;
    }

    scoreMoves(myLegal, myLegalCount, entryHash==currentHash, entry.bestMove);
    sortMoves(myLegal, myLegalCount);

    int bestScore = (turn==0) ? -999999 : 999999;
    Move bestMv = myLegal[0];

    if(turn==0){
        for(int i=0;i<myLegalCount;i++){
            makeMove(myLegal[i]);
            int eval=minimax(alpha,beta,depth-1);
            undoMove();
            if(eval>bestScore){ bestScore=eval; bestMv=myLegal[i]; }
            alpha=max(alpha,eval);
            if(beta<=alpha) break;
        }
    } else {
        for(int i=0;i<myLegalCount;i++){
            makeMove(myLegal[i]);
            int eval=minimax(alpha,beta,depth-1);
            undoMove();
            if(eval<bestScore){ bestScore=eval; bestMv=myLegal[i]; }
            beta=min(beta,eval);
            if(beta<=alpha) break;
        }
    }

    int8_t flag;
    if(bestScore <= originalAlpha) flag = 2;
    else if(bestScore >= beta)     flag = 1;
    else                           flag = 0;

    entry.hash.store(currentHash);
    entry.eval.store(bestScore);
    entry.depth.store(depth);
    entry.flag.store(flag);
    entry.bestMove = bestMv;

    return bestScore;
}
Move bestMove(int maxDepth) {
    stopSearch.store(false);
    nodeCount.store(0);
    auto startTime = std::chrono::steady_clock::now();
 
    Move best = legalMoves[0];
    int numThreads  = max(1, (int)std::thread::hardware_concurrency() / 2);
    int helperCount = numThreads - 1;
 
    Move myLegal[256]; int myLegalCount = legalCount;
    for (int i = 0; i < legalCount; i++) myLegal[i] = legalMoves[i];
 
    auto helperFn = [&](int threadId) {
        Board copy;
        copy.copyForThread(*this);
        int helperDepth = 1;
        while (!stopSearch.load(std::memory_order_relaxed)) {
            copy.negamax(-999999, 999999, helperDepth);
            helperDepth++;
            if (helperDepth > maxDepth) helperDepth = 1;
        }
    };
 
    std::vector<std::thread> helpers;
    for (int i = 0; i < helperCount; i++) helpers.emplace_back(helperFn, i);
 
    int prevScore = 0;
 
    for (int depth = 1; depth <= maxDepth; depth++) {
        TTentry& rootEntry = tt[currentHash & ((1 << 22) - 1)];
        bool ttValid = (rootEntry.hash.load() == currentHash);
        Move ttMove  = rootEntry.bestMove;
 
        Move sorted[256];
        for (int i = 0; i < myLegalCount; i++) sorted[i] = myLegal[i];
        scoreMoves(sorted, myLegalCount, ttValid, ttMove);
        sortMoves(sorted, myLegalCount);
 
        int aspAlpha, aspBeta;
        if (depth <= 4) {
            aspAlpha = -999999;
            aspBeta  =  999999;
        } else {
            aspAlpha = prevScore - 50;
            aspBeta  = prevScore + 50;
        }
 
        int  bestScore = -999999;
        Move depthBest = sorted[0];
        int  failCount = 0;
 
        while (true) {
            int  iterBest  = -999999;
            Move iterMove  = sorted[0];
 
            for (int i = 0; i < myLegalCount; i++) {
                makeMove(sorted[i]);
                int score = -negamax(-aspBeta, -aspAlpha, depth - 1);
                undoMove();
                if (score > iterBest) {
                    iterBest = score;
                    iterMove = sorted[i];
                }
            }
 
            if (iterBest <= aspAlpha) {
                failCount++;
                int delta = 50 * (1 << failCount);
                aspAlpha = max(prevScore - delta, -999999);
            } else if (iterBest >= aspBeta) {
                failCount++;
                int delta = 50 * (1 << failCount);
                aspBeta = min(prevScore + delta, 999999);
            } else {
                bestScore = iterBest;
                depthBest = iterMove;
                break;
            }
 
            if (aspAlpha == -999999 && aspBeta == 999999) {
                bestScore = iterBest;
                depthBest = iterMove;
                break;
            }
        }
 
        prevScore = bestScore;
        best      = depthBest;
 
        auto now       = std::chrono::steady_clock::now();
        double elapsed = std::chrono::duration<double>(now - startTime).count();
        long long nodes = nodeCount.load();
        long long nps   = elapsed > 0 ? (long long)(nodes / elapsed) : 0;
        char letters[]  = {'a','b','c','d','e','f','g','h'};
        std::cout << "depth " << depth
                  << " score " << bestScore
                  << " move "  << letters[best.from % 8] << (best.from / 8) + 1
                               << letters[best.to   % 8] << (best.to   / 8) + 1
                  << " nodes " << nodes
                  << " nps "   << nps
                  << " time "  << (int)(elapsed * 1000) << "ms\n";
    }
 
    stopSearch.store(true);
    for (auto& t : helpers) t.join();
 
    auto end     = std::chrono::steady_clock::now();
    double total = std::chrono::duration<double>(end - startTime).count();
    long long nodes = nodeCount.load();
    std::cout << "total nodes " << nodes
              << " nps "  << (long long)(nodes / total)
              << " time " << (int)(total * 1000) << "ms\n";
 
    return best;
}
 
bool gameOver(){
    if(legalCount==0){return true;}
    if(isDrawByRep()){return true;}
    return false;

}
void printSqToNot(Move m){
    char letters[]= {'a','b','c','d','e','f','g','h'};
    std::cout<<letters[m.from%8]<<(m.from/8)+1<<">>"<<letters[m.to%8]<<(m.to/8)+1<<endl;
}
void genCastleMoves(){
    if (isCheck() )return;
if(turn==0){
    if(castleRights&1&&(!(all_pieces&((1ULL<<1)|(1ULL<<2)|(1ULL<<3)))&&
    (!(generateAttackMask(true)&((1ULL<<2)|(1ULL<<3)))))){
        addMove(moves,count,4,2,false,0,true);
 }
if(castleRights&2&&(!(all_pieces&((1ULL<<5)|(1ULL<<6)))&&
    (!(generateAttackMask(true)&((1ULL<<5)|(1ULL<<6)))))){
        addMove(moves,count,4,6,false,0,true);
 }
}
else{

if(castleRights&4&&(!(all_pieces&((1ULL<<59)|(1ULL<<58)|(1ULL<<57)))&&
    (!(generateAttackMask(false)&((1ULL<<59)|(1ULL<<58)))))){
        addMove(moves,count,60,58,false,0,true);
 }
if(castleRights&8&&(!(all_pieces&((1ULL<<61)|(1ULL<<62)))&&
    (!(generateAttackMask(false)&((1ULL<<61)|(1ULL<<62)))))){
        addMove(moves,count,60,62,false,0,true);
 }

}

}
void initZobristTables(){
    std::random_device rd;
    std::mt19937_64 gen(rd());

    std::uniform_int_distribution<uint64_t> dist;
    for(int i=0;i<64;i++){
        for(int j=0;j<12;j++){
            pieceHash[i][j]=dist(gen);
        }
    }
    blackToMove=dist(gen);
    for(int i=0; i<16; i++){
        castleHash[i]=dist(gen);
    }
    for(int i=0;i<8;i++){
        enpassHash[i]=dist(gen);
    }
}
uint64_t calcZobristHash(){
    uint64_t h=0ULL;
    for(int i=0;i<64;i++){
        int p=getPiece(i);
        if(p>0){
            h^=pieceHash[i][p-1];

        }
        else if(p<0){
            h^=pieceHash[i][p*-1+5];
        }
    }
    if (turn==1){
        h^=blackToMove;
    }
    h^=castleHash[castleRights];
    if (enpass_target!=-1){
        h^=enpassHash[enpass_target%8];
    }
    return h;
}
bool isDrawByRep(){
    int count = 0;
    for(int i=ply-2;i>=0;i-=2){
        if(hashHistory[i]==currentHash){
            count++;
            if(count>=2) return true; 
        }
    }
    return false;
}
void initTT(){
    memset(tt,0,sizeof(tt));
}
void scoreMoves(Move arr [],int cnt, bool ttValid,Move ttMove){
    static const int val[] = {0,100,320,330,500,900,20000};
    for(int i=0;i<cnt;i++){
        if(ttValid&&arr[i].from==ttMove.from&&arr[i].to==ttMove.to){
            arr[i].score=100000;
        }
        else if(arr[i].capture){
            int a=abs(getPiece(arr[i].from));
            int v=abs(getPiece(arr[i].to));
            arr[i].score=val[v]*10-val[a];
        }

        else{
            arr[i].score=0;
        }
    }
}
void sortMoves(Move arr[], int cnt){
    for(int i=1;i<cnt;i++){
        Move key=arr[i];
        int j=i-1;
        while(j>=0 && arr[j].score < key.score){
            arr[j+1]=arr[j];
            j--;
        }
        arr[j+1]=key;
    }
}
int quiesce(int alpha, int beta){
    nodeCount.fetch_add(1, std::memory_order_relaxed);

    int stand_pat = evaluate();
    if(stopSearch.load(std::memory_order_relaxed)) return stand_pat;
    if(stand_pat >= beta) return beta;
    if(stand_pat > alpha) alpha = stand_pat;

    int savedCount = count;
    count = 0;
    generateAllMoves();
    int pseudoCount = count;
    Move pseudo[256];
    for(int i=0;i<pseudoCount;i++) pseudo[i] = moves[i];
    count = savedCount;

    int myTurn = turn;
    Move myCaps[256]; int myCapCount = 0;
    for(int i=0;i<pseudoCount;i++){
        if(!pseudo[i].capture) continue;
        makeMove(pseudo[i]);
        bool inCheck = (myTurn==0)
            ? (white_king & generateAttackMask(true))
            : (black_king & generateAttackMask(false));
        if(!inCheck) myCaps[myCapCount++] = pseudo[i];
        undoMove();
    }

    Move dummy{};
    scoreMoves(myCaps, myCapCount, false, dummy);
    sortMoves(myCaps, myCapCount);

    for(int i = 0; i < myCapCount; i++){
        if(stopSearch.load(std::memory_order_relaxed)) return alpha;
        makeMove(myCaps[i]);
        int eval = -quiesce(-beta, -alpha);
        undoMove();
        if(eval >= beta) return beta;
        if(eval > alpha) alpha = eval;
    }
    return alpha;
}
int negamax(int alpha, int beta, int depth) {
    nodeCount.fetch_add(1, std::memory_order_relaxed);
    if (stopSearch.load(std::memory_order_relaxed)) return 0;
    if (isDrawByRep()) return 0;
 
    bool inCheckNow = isCheck();
    if (inCheckNow) depth++;
 
    int originalAlpha = alpha;
    uint64_t ttIdx   = currentHash & ((1 << 22) - 1);
    TTentry& entry   = tt[ttIdx];
    uint64_t entryHash  = entry.hash .load(std::memory_order_relaxed);
    int      entryEval  = entry.eval .load(std::memory_order_relaxed);
    int      entryDepth = entry.depth.load(std::memory_order_relaxed);
    int8_t   entryFlag  = entry.flag .load(std::memory_order_relaxed);
    Move     entryMove  = entry.bestMove;
 
    if (entryHash == currentHash && entryDepth >= depth) {
        if (entryFlag == 0) return entryEval;
        if (entryFlag == 1) alpha = max(alpha, entryEval);
        if (entryFlag == 2) beta  = min(beta,  entryEval);
        if (alpha >= beta)  return entryEval;
    }
 
    if (depth == 0) return quiesce(alpha, beta);
 
    if (depth >= 3 && !inCheckNow && __builtin_popcountll(all_pieces) > 10) {
        int epSave = enpass_target;
        turn ^= 1;
        currentHash ^= blackToMove;
        if (enpass_target != -1) {
            currentHash ^= enpassHash[enpass_target % 8];
            enpass_target = -1;
        }
        int nullScore = -negamax(-beta, -beta + 1, depth - 3);
        turn ^= 1;
        currentHash ^= blackToMove;
        enpass_target = epSave;
        if (enpass_target != -1) currentHash ^= enpassHash[enpass_target % 8];
        if (nullScore >= beta) return beta;
    }
 
    Move local[256]; int localCount = 0;
    int savedCount = count;
    count = 0;
    generateAllMoves();
    int pseudoCount = count;
    Move pseudo[256];
    for (int i = 0; i < pseudoCount; i++) pseudo[i] = moves[i];
    count = savedCount;
 
    int myTurn = turn;
    for (int i = 0; i < pseudoCount; i++) {
        makeMove(pseudo[i]);
        bool illegal = (myTurn == 0)
            ? (white_king & generateAttackMask(true))
            : (black_king & generateAttackMask(false));
        if (!illegal) local[localCount++] = pseudo[i];
        undoMove();
    }
 
    if (localCount == 0) {
        if (inCheckNow) return -999999 + ply;
        return 0;
    }
 
    scoreMoves(local, localCount, entryHash == currentHash, entryMove);
    sortMoves(local, localCount);
 
    int  bestScore       = -999999;
    Move bestMv          = local[0];
    bool searchCompleted = true;
    int  movesSearched   = 0;
 
    for (int i = 0; i < localCount; i++) {
        if (stopSearch.load(std::memory_order_relaxed)) { searchCompleted = false; break; }
 
        bool isCapture = local[i].capture;
        bool isPromo   = local[i].promotion != 0;
 
        makeMove(local[i]);
        bool givesCheck = isCheck();
 
        int eval;
 
        if (depth >= 3
            && movesSearched >= 4
            && !isCapture
            && !isPromo
            && !inCheckNow
            && !givesCheck)
        {
            int R = (movesSearched < 9) ? 1 : 2;
            eval = -negamax(-alpha - 1, -alpha, depth - 1 - R);
            if (eval > alpha) {
                eval = -negamax(-beta, -alpha, depth - 1);
            }
        } else {
            eval = -negamax(-beta, -alpha, depth - 1);
        }
 
        undoMove();
        movesSearched++;
 
        if (eval > bestScore) { bestScore = eval; bestMv = local[i]; }
        if (eval > alpha)       alpha = eval;
        if (alpha >= beta)      break;
    }
 
    if (searchCompleted && bestScore > -999999) {
        int8_t flag;
        if      (bestScore <= originalAlpha) flag = 2;
        else if (bestScore >= beta)          flag = 1;
        else                                 flag = 0;
 
        if (entryDepth <= depth) {
            entry.hash .store(currentHash, std::memory_order_relaxed);
            entry.eval .store(bestScore,   std::memory_order_relaxed);
            entry.depth.store(depth,       std::memory_order_relaxed);
            entry.flag .store(flag,        std::memory_order_relaxed);
            entry.bestMove = bestMv;
        }
    }
 
    return bestScore;
}
int convertMove(std::string square) {
    int col = square[0] - 'a';
    int row = square[1] - '1'; 
    return row*8+col;
}
void makeLegalMove(int from, int to){
    for(int i=0;i<legalCount;i++){
        if(legalMoves[i].from==from&&legalMoves[i].to==to){
            makeMove(legalMoves[i]);
            break;
        }
    }
}
int evaluate(){
    return (turn == 0) ? symbolicEval() : -symbolicEval();
}
void copyForThread(const Board& src){
    white_pawns=src.white_pawns; white_knights=src.white_knights;
    white_bishops=src.white_bishops; white_rooks=src.white_rooks;
    white_queen=src.white_queen; white_king=src.white_king;
    black_pawns=src.black_pawns; black_knights=src.black_knights;
    black_bishops=src.black_bishops; black_rooks=src.black_rooks;
    black_queen=src.black_queen; black_king=src.black_king;
    white_pieces=src.white_pieces; black_pieces=src.black_pieces;
    all_pieces=src.all_pieces;
    enpass_target=src.enpass_target; turn=src.turn; count=src.count;
    legalCount=src.legalCount; captureCount=src.captureCount;
    castleRights=src.castleRights; ply=src.ply; currentHash=src.currentHash;
    for(int i=0;i<legalCount;i++)   legalMoves[i]=src.legalMoves[i];
    for(int i=0;i<captureCount;i++) captureMoves[i]=src.captureMoves[i];
    for(int i=0;i<=ply;i++)         hashHistory[i]=src.hashHistory[i];
    for(int i=0;i<ply;i++)          History[i]=src.History[i];
    for(int i=0;i<64;i++){
        knightTable[i]=src.knightTable[i];
        kingTable[i]=src.kingTable[i];
        rookTable[i]=src.rookTable[i];
        bishopTable[i]=src.bishopTable[i];
        for(int j=0;j<12;j++) pieceHash[i][j]=src.pieceHash[i][j];
    }
    for(int i=0;i<16;i++) castleHash[i]=src.castleHash[i];
    for(int i=0;i<8;i++)  enpassHash[i]=src.enpassHash[i];
    blackToMove=src.blackToMove;
}
};

uint64_t Board::rookAttackTable[64][1<<13];
uint64_t Board::bishopAttackTable[64][1<<11];
TTentry Board::tt[1<<22];
std::atomic<bool> Board::stopSearch{false};
std::atomic<long long> Board::nodeCount{0};
int main() {
    std::cout<<"Start"<<endl;
    Board board;
    board.initBoard();

    //board.initMagics(13,11);
    //board.printMagics();
    //board.initMagicNumsAndShifts();
    //board.printBB(board.rookAttackTable[63][board.magicIndex(board.rookTable[63].magic,board.all_pieces&board.rookTable[63].mask,board.rookTable[63].shift)]);
    //board.printBB(board.bishopAttackTable[1][board.magicIndex(board.bishopTable[1].magic,0ULL,board.bishopTable[1].shift)]);
    /* board.printBoard();
    board.generateAllMoves();
    board.makeMove(board.moves[10]);
    board.printBoard();
    board.printBB(board.generateAttackMask(true));
    board.generateAllMoves();
    board.makeMove(board.moves[10]);
    board.printBoard();

    board.printBB(board.generateAttackMask(true));
 */
/* 
    board.printBB(board.generateAttackMask(true));
        board.generateAllMoves();

        board.makeMove(board.moves[10]);
            board.generateAllMoves();

    board.makeMove(board.moves[10]);
        board.printBB(board.generateAttackMask(true));

 */
//board.printBB(board.generateAttackMask(true));
//printBB(board.generateAttackMask(false));
//std::cout<<board.isCheck();

/* while(!board.gameOver()){
    board.printBoard();
    board.makeMove(board.bestMove(5));

} */
/*

for (int i=0;i<100;i++){
    board.generateLegals();
    board.makeMove(board.legalMoves[0]);

    if(i%10==0){
        std::cout<<"#"<<i<<endl;
        board.printBoard();
        //board.printMoves(board.legalMoves,board.legalCount);
        std::cout<<endl;
        std::cout<<board.simpleEval()<<endl;
    }
}
*/
board.generateLegals();
int a;
std::cout<<"Make bot play self (0), Play against bot (1)"<<endl;
std::cin>>a;
if(a==0){
while(!board.gameOver()){
    Move m=board.bestMove(5);
    board.makeMove(m);
    board.printBoard();
    board.printSqToNot(m);


    board.generateLegals();
std::cout<<"Count: "<<board.count<<endl;

std::cout<<"LegalCount: "<<board.legalCount<<endl;
std::cout<<"CaptureCount: "<<board.captureCount<<endl;

std::cout<<"isCheck: "<<board.isCheck()<<endl;
std::cout<<"isOver: "<<board.gameOver()<<endl;
    std::cout<<(int)m.from<<">>"<<(int)m.to<<endl;
    std::cout<<"Eval: "<<board.symbolicEval()<<endl;
}
}
else{
    std::string from;
    std::string to;
    board.printBoard();
    while(!board.gameOver()){
        board.generateLegals();
        std::cout<<"Enter from square (Ex: A1)"<<endl;
        std::cin>>from;
        std::cout<<"Enter to square (Ex: A4)"<<endl;
        std::cin>>to;
        board.makeLegalMove(board.convertMove(from),board.convertMove(to));
        board.printBoard();
        board.generateLegals();
        Move m=board.bestMove(5);
        board.makeMove(m);   
        std::cout<<"Computer makes move!"<<endl;
        board.printBoard();

    }
}
}
