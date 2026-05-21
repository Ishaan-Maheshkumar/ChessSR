import os
import sys
from pathlib import Path

import numpy as np
import pandas as pd
import chess
from pysr import PySRRegressor

CSV_PATH = "random_evals.csv"
MAX_FEATURES = 44
PYSR_ITERATIONS = 10000000
MAX_POINTS = 50_000

PROJECT_ROOT = Path.home() / "Chess"
OUTPUT_DIR = PROJECT_ROOT / "outputs"
OUTPUT_DIR.mkdir(parents=True, exist_ok=True)

FILE_MASKS = [0x0101010101010101 << f for f in range(8)]
KNIGHT_DXDY = [
    (2, 1), (2, -1), (1, 2), (-1, 2),
    (-2, 1), (-2, -1), (-1, -2), (1, -2),
]


def pop(bb: int) -> int:
    return bin(bb).count("1")


def knight_attacks(sq: int) -> int:
    bb = 0
    r, f = divmod(sq, 8)
    for dr, df in KNIGHT_DXDY:
        nr, nf = r + dr, f + df
        if 0 <= nr < 8 and 0 <= nf < 8:
            bb |= 1 << (nr * 8 + nf)
    return bb


KNIGHT_TABLE = [knight_attacks(sq) for sq in range(64)]

PAWN_PSQT = [
     0,  0,  0,  0,  0,  0,  0,  0,
     5, 10, 10,-20,-20, 10, 10,  5,
     5, -5,-10,  0,  0,-10, -5,  5,
     0,  0,  0, 20, 20,  0,  0,  0,
     5,  5, 10, 25, 25, 10,  5,  5,
    10, 10, 20, 30, 30, 20, 10, 10,
    50, 50, 50, 50, 50, 50, 50, 50,
     0,  0,  0,  0,  0,  0,  0,  0,
]

KNIGHT_PSQT = [
    -50,-40,-30,-30,-30,-30,-40,-50,
    -40,-20,  0,  0,  0,  0,-20,-40,
    -30,  0, 10, 15, 15, 10,  0,-30,
    -30,  5, 15, 20, 20, 15,  5,-30,
    -30,  0, 15, 20, 20, 15,  0,-30,
    -30,  5, 10, 15, 15, 10,  5,-30,
    -40,-20,  0,  5,  5,  0,-20,-40,
    -50,-40,-30,-30,-30,-30,-40,-50,
]

BISHOP_PSQT = [
    -20,-10,-10,-10,-10,-10,-10,-20,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -10,  0,  5, 10, 10,  5,  0,-10,
    -10,  5,  5, 10, 10,  5,  5,-10,
    -10,  0, 10, 10, 10, 10,  0,-10,
    -10, 10, 10, 10, 10, 10, 10,-10,
    -10,  5,  0,  0,  0,  0,  5,-10,
    -20,-10,-10,-10,-10,-10,-10,-20,
]

ROOK_PSQT = [
      0,  0,  0,  0,  0,  0,  0,  0,
      5, 10, 10, 10, 10, 10, 10,  5,
     -5,  0,  0,  0,  0,  0,  0, -5,
     -5,  0,  0,  0,  0,  0,  0, -5,
     -5,  0,  0,  0,  0,  0,  0, -5,
     -5,  0,  0,  0,  0,  0,  0, -5,
     -5,  0,  0,  0,  0,  0,  0, -5,
      0,  0,  0,  5,  5,  0,  0,  0,
]

KING_PSQT = [
     20, 30, 10,  0,  0, 10, 30, 20,
     20, 20,  0,  0,  0,  0, 20, 20,
    -10,-20,-20,-20,-20,-20,-20,-10,
    -20,-30,-30,-40,-40,-30,-30,-20,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
]

CENTRE = (1 << 27) | (1 << 28) | (1 << 35) | (1 << 36)
EXT_CENTRE = (
    (1 << 18) | (1 << 19) | (1 << 20) | (1 << 21) |
    (1 << 26) | (1 << 29) | (1 << 34) | (1 << 37) |
    (1 << 42) | (1 << 43) | (1 << 44) | (1 << 45)
)


def bb_squares(bb: int):
    while bb:
        lsb = bb & -bb
        yield lsb.bit_length() - 1
        bb ^= lsb


def doubled_pawns(pawn_bb: int) -> int:
    d = 0
    for fm in FILE_MASKS:
        cnt = pop(pawn_bb & fm)
        if cnt > 1:
            d += cnt - 1
    return d


def isolated_pawns(pawn_bb: int) -> int:
    iso = 0
    for f in range(8):
        fm = FILE_MASKS[f]
        if not (pawn_bb & fm):
            continue
        adj = 0
        if f > 0:
            adj |= FILE_MASKS[f - 1]
        if f < 7:
            adj |= FILE_MASKS[f + 1]
        if not (pawn_bb & adj):
            iso += 1
    return iso


def passed_pawns(my_pawns: int, their_pawns: int, is_white: bool) -> int:
    cnt = 0
    for sq in bb_squares(my_pawns):
        f, r = sq % 8, sq // 8
        front = 0
        rng = range(r + 1, 8) if is_white else range(r - 1, -1, -1)
        for rr in rng:
            if f > 0:
                front |= 1 << (rr * 8 + f - 1)
            front |= 1 << (rr * 8 + f)
            if f < 7:
                front |= 1 << (rr * 8 + f + 1)
        if not (their_pawns & front):
            cnt += 1
    return cnt


def pawn_advancement(pawn_bb: int, is_white: bool) -> int:
    total = 0
    for sq in bb_squares(pawn_bb):
        r = sq // 8
        total += r if is_white else (7 - r)
    return total


def king_shield(king_bb: int, pawn_bb: int, is_white: bool) -> int:
    ksq = king_bb.bit_length() - 1
    kf, kr = ksq % 8, ksq // 8
    shield = 0
    for df in [-1, 0, 1]:
        f2 = kf + df
        if not (0 <= f2 < 8):
            continue
        r_front = kr + 1 if is_white else kr - 1
        if 0 <= r_front < 8 and (pawn_bb & (1 << (r_front * 8 + f2))):
            shield += 1
    return shield


def king_centrality(king_bb: int) -> int:
    sq = king_bb.bit_length() - 1
    f, r = sq % 8, sq // 8
    return -(abs(f - 3) + abs(r - 3))


def knight_mobility(knight_bb: int, same_bb: int) -> int:
    total = 0
    for sq in bb_squares(knight_bb):
        total += pop(KNIGHT_TABLE[sq] & ~same_bb)
    return total


def rook_open_file(rook_bb: int, my_pawns: int, their_pawns: int) -> int:
    cnt = 0
    for sq in bb_squares(rook_bb):
        fm = FILE_MASKS[sq % 8]
        if not (my_pawns & fm):
            cnt += 1
        if not (their_pawns & fm):
            cnt += 1
    return cnt


def rook_seventh(rook_bb: int, is_white: bool) -> int:
    rank7 = 0x00FF000000000000 if is_white else 0x000000000000FF00
    return pop(rook_bb & rank7)


def knight_outpost(knight_bb: int, their_pawns: int, is_white: bool) -> int:
    OUTPOST_W = 0x0000FFFFFF000000
    OUTPOST_B = 0x000000FFFFFF0000
    candidates = knight_bb & (OUTPOST_W if is_white else OUTPOST_B)
    cnt = 0
    for sq in bb_squares(candidates):
        f, r = sq % 8, sq // 8
        attacked = False
        if is_white:
            if f > 0 and (their_pawns & (1 << ((r - 1) * 8 + f - 1))):
                attacked = True
            if f < 7 and (their_pawns & (1 << ((r - 1) * 8 + f + 1))):
                attacked = True
        else:
            if f > 0 and (their_pawns & (1 << ((r + 1) * 8 + f - 1))):
                attacked = True
            if f < 7 and (their_pawns & (1 << ((r + 1) * 8 + f + 1))):
                attacked = True
        if not attacked:
            cnt += 1
    return cnt


def connected_rooks(rook_bb: int) -> int:
    sqs = list(bb_squares(rook_bb))
    if len(sqs) < 2:
        return 0
    a, b = sqs[0], sqs[1]
    return 1 if (a // 8 == b // 8 or a % 8 == b % 8) else 0


def dev_score(knight_bb: int, bishop_bb: int, is_white: bool) -> int:
    home = 0x00000000000000FF if is_white else 0xFF00000000000000
    return pop((knight_bb | bishop_bb) & ~home)


def psqt_sum(bb: int, table, flip: bool) -> int:
    total = 0
    for sq in bb_squares(bb):
        total += table[sq ^ 56 if flip else sq]
    return total


def extract_features(board: chess.Board) -> np.ndarray:
    wp = int(board.occupied_co[chess.WHITE] & board.pawns)
    wn = int(board.occupied_co[chess.WHITE] & board.knights)
    wb = int(board.occupied_co[chess.WHITE] & board.bishops)
    wr = int(board.occupied_co[chess.WHITE] & board.rooks)
    wq = int(board.occupied_co[chess.WHITE] & board.queens)
    wk = int(board.occupied_co[chess.WHITE] & board.kings)

    bp = int(board.occupied_co[chess.BLACK] & board.pawns)
    bn = int(board.occupied_co[chess.BLACK] & board.knights)
    bb = int(board.occupied_co[chess.BLACK] & board.bishops)
    br = int(board.occupied_co[chess.BLACK] & board.rooks)
    bq = int(board.occupied_co[chess.BLACK] & board.queens)
    bk = int(board.occupied_co[chess.BLACK] & board.kings)

    wpieces = int(board.occupied_co[chess.WHITE])
    bpieces = int(board.occupied_co[chess.BLACK])

    w_mat = 100 * pop(wp) + 320 * pop(wn) + 330 * pop(wb) + 500 * pop(wr) + 900 * pop(wq)
    b_mat = 100 * pop(bp) + 320 * pop(bn) + 330 * pop(bb) + 500 * pop(br) + 900 * pop(bq)
    total_mat = w_mat + b_mat
    phase = total_mat / 7800.0

    f = [
        pop(wp) - pop(bp),
        pop(wn) - pop(bn),
        pop(wb) - pop(bb),
        pop(wr) - pop(br),
        pop(wq) - pop(bq),
        pop(wpieces),
        pop(bpieces),
        doubled_pawns(wp) - doubled_pawns(bp),
        isolated_pawns(wp) - isolated_pawns(bp),
        passed_pawns(wp, bp, True) - passed_pawns(bp, wp, False),
        pawn_advancement(wp, True) - pawn_advancement(bp, False),
        king_shield(wk, wp, True) - king_shield(bk, bp, False),
        king_centrality(wk) - king_centrality(bk),
        knight_mobility(wn, wpieces) - knight_mobility(bn, bpieces),
        rook_open_file(wr, wp, bp) - rook_open_file(br, bp, wp),
        int(pop(wb) >= 2) - int(pop(bb) >= 2),
        pop(wp & CENTRE) - pop(bp & CENTRE),
        pop(wp & EXT_CENTRE) - pop(bp & EXT_CENTRE),
        1.0 if board.turn == chess.WHITE else -1.0,
        float(w_mat),
        float(b_mat),
        float(w_mat - b_mat),
        float(total_mat),
        psqt_sum(wp, PAWN_PSQT, False)   - psqt_sum(bp, PAWN_PSQT, True),
        psqt_sum(wn, KNIGHT_PSQT, False) - psqt_sum(bn, KNIGHT_PSQT, True),
        psqt_sum(wb, BISHOP_PSQT, False) - psqt_sum(bb, BISHOP_PSQT, True),
        psqt_sum(wr, ROOK_PSQT, False)   - psqt_sum(br, ROOK_PSQT, True),
        psqt_sum(wk, KING_PSQT, False)   - psqt_sum(bk, KING_PSQT, True),
        (pop(wp) - pop(bp)) * (pop(wr) - pop(br)),
        (pop(wn) - pop(bn)) * (pop(wq) - pop(bq)),
        (pop(wb) - pop(bb)) * (pop(wr) - pop(br)),
        (pop(wp) - pop(bp)) ** 2,
        (pop(wr) - pop(br)) ** 2,
        phase,
        (w_mat - b_mat) * phase,
        (king_centrality(wk) - king_centrality(bk)) * (1.0 - phase),
        rook_seventh(wr, True) - rook_seventh(br, False),
        pop(wp & 0x00FF000000000000) - pop(bp & 0x000000000000FF00),
        knight_outpost(wn, bp, True) - knight_outpost(bn, wp, False),
        connected_rooks(wr) - connected_rooks(br),
        dev_score(wn, wb, True) - dev_score(bn, bb, False),
    ]
    return np.array(f, dtype=np.float32)


FEATURE_NAMES = [
    "pawn_diff", "knight_diff", "bishop_diff", "rook_diff", "queen_diff",
    "white_piece_count", "black_piece_count",
    "doubled_pawn_diff", "isolated_pawn_diff", "passed_pawn_diff",
    "pawn_advancement_diff", "king_shield_diff", "king_centrality_diff",
    "knight_mobility_diff", "rook_open_file_diff", "bishop_pair_diff",
    "centre_pawn_diff", "ext_centre_pawn_diff", "turn",
    "white_material", "black_material", "material_balance", "total_material",
    "pawn_psqt_diff", "knight_psqt_diff", "bishop_psqt_diff",
    "rook_psqt_diff", "king_psqt_diff",
    "pawn_x_rook_diff", "knight_x_queen_diff", "bishop_x_rook_diff",
    "pawn_sq_diff", "rook_sq_diff", "phase",
    "material_x_phase", "king_centrality_x_endgame",
    "rook_seventh_diff", "pawn_seventh_diff",
    "knight_outpost_diff", "connected_rooks_diff", "development_diff",
]

assert len(FEATURE_NAMES) == 41


def load_csv(path: str):
    df = pd.read_csv(path)
    fen_col = df.columns[0]
    eval_col = df.columns[1]

    rows, labels = [], []
    skipped = 0

    for _, row in df.iterrows():
        fen = str(row[fen_col]).strip()
        raw = str(row[eval_col]).strip()
        try:
            val = float(raw)
        except ValueError:
            skipped += 1
            continue

        val = max(min(val, 1000.0), -1000.0)

        try:
            board = chess.Board(fen)
        except Exception:
            skipped += 1
            continue

        rows.append(extract_features(board))
        labels.append(val)

    print(f"Loaded {len(rows)} positions ({skipped} skipped)")
    X = np.array(rows, dtype=np.float32)
    y = np.array(labels, dtype=np.float32)
    return X, y


def run_pysr(X: np.ndarray, y: np.ndarray) -> PySRRegressor:
    n_features = X.shape[1]
    assert n_features <= len(FEATURE_NAMES)

    print(f"X shape: {X.shape}, y shape: {y.shape}")
    print(f"Using {n_features} features")

    model = PySRRegressor(
        niterations=PYSR_ITERATIONS,
        binary_operators=["+", "-", "*", "/"],
        unary_operators=["square","abs"],
        populations=128,
        population_size=50,
        ncycles_per_iteration=3000,
        maxsize=30,
        parsimony=1e-5,
	batching=True,
	batch_size=4096,
        model_selection="best",
        verbosity=1,
	procs=32,
	parallelism="multithreading",
        random_state=42,
        select_k_features=None,
        progress=True,
        warm_start=False,
	loss="L1DistLoss()",
    )

    model.fit(X, y, variable_names=FEATURE_NAMES[:n_features])
    return model


def format_for_cpp(model: PySRRegressor) -> str:
    best = model.get_best()
    eq = str(best["equation"])

    lines = [
        "int symbolicEval(const Position& pos) {",
        "    Features feat = extractFeatures(pos);",
        "    float* x = feat.f;",
        f"    float result = {eq};",
        "    return (int)result;",
        "}",
    ]
    return "\n".join(lines)


if __name__ == "__main__":
    csv_path = PROJECT_ROOT / CSV_PATH
    if not csv_path.exists():
        print(f"CSV not found: {csv_path}")
        sys.exit(1)

    X, y = load_csv(str(csv_path))
    print(f"Features: {X.shape[1]}   Eval range: {y.min():.0f} to {y.max():.0f} cp")

    if len(y) > MAX_POINTS:
        idx = np.random.choice(len(y), size=MAX_POINTS, replace=False)
        X = X[idx]
        y = y[idx]
        print(f"Subsampled to {len(y)} positions for PySR.")

    n_used = min(MAX_FEATURES, X.shape[1])
    X_used = X[:, :n_used]

    model = run_pysr(X_used, y)

    print(model)
    print(model.equations_[["complexity", "loss", "equation"]].to_string(index=False))

    cpp = format_for_cpp(model)
    out_cpp = PROJECT_ROOT / "best_formula.txt"
    with open(out_cpp, "w") as fh:
        fh.write(cpp)
    print("\n--- C++ formula written to best_formula.txt ---")
    print(cpp)
