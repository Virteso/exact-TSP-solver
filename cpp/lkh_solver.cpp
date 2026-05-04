extern "C" {
#include "LKH.h"
#include "Genetic.h"
}

#include "lkh_solver.h"
#include <cstring>
#include <cmath>
#include <stdexcept>

#define LKH_Link(a, b) { (a)->Suc = (b); (b)->Pred = (a); }

static Node *local_FirstNode = nullptr;
static Node *local_NodeSet = nullptr;
static int *local_CostMatrix = nullptr;
static CostFunction local_Distance = nullptr;
static CostFunction local_D_func = nullptr;
static CostFunction local_C_func = nullptr;
static CostFunction local_c_func = nullptr;

static void setParameterDefaults(int dimension, int max_trials,
                                  int max_candidates, int runs,
                                  int move_type, unsigned seed,
                                  double time_limit)
{
    Dimension = dimension;
    DimensionSaved = dimension;
    Dim = dimension;
    ProblemType = TSP;
    WeightType = EXPLICIT;
    CoordType = NO_COORDS;

    AscentCandidates = 50;
    BackboneTrials = 0;
    Backtracking = 0;
    CandidateSetSymmetric = 0;
    CandidateSetType = ALPHA;
    Crossover = ERXT;
    DelaunayPartitioning = 0;
    DelaunayPure = 0;
    DemandDimension = 1;
    DistanceLimit = 1e308;
    Drones = 1;
    Endurance = 0.0;
    Excess = -1.0;
    ExternalSalesmen = 0;
    ExtraCandidates = 0;
    ExtraCandidateSetSymmetric = 0;
    ExtraCandidateSetType = QUADRANT;
    Gain23Used = 1;
    GainCriterionUsed = 1;
    GridSize = 1000000.0;
    InitialPeriod = -1;
    InitialStepSize = 0;
    InitialTourAlgorithm = WALK;
    InitialTourFraction = 1.0;
    KarpPartitioning = 0;
    KCenterPartitioning = 0;
    KMeansPartitioning = 0;
    Kicks = 1;
    KickType = 0;
    MaxBreadth = INT_MAX;
    MaxCandidates = max_candidates;
    MaxPopulationSize = 0;
    MaxSwaps = -1;
    MaxTrials = max_trials;
    MoorePartitioning = 0;
    MoveType = move_type;
    MoveTypeSpecial = 0;
    MTSPDepot = 1;
    MTSPMinSize = 1;
    MTSPMaxSize = -1;
    MTSPObjective = -1;
    NonsequentialMoveType = -1;
    Optimum = MINUS_INFINITY;
    PatchingA = 1;
    PatchingC = 0;
    PatchingAExtended = 0;
    PatchingARestricted = 0;
    PatchingCExtended = 0;
    PatchingCRestricted = 0;
    Precision = 100;
    Probability = 100;
    POPMUSIC_InitialTour = 0;
    POPMUSIC_MaxNeighbors = 5;
    POPMUSIC_SampleSize = 10;
    POPMUSIC_Solutions = 50;
    POPMUSIC_Trials = 1;
    Recombination = IPT;
    RestrictedSearch = 1;
    RohePartitioning = 0;
    Runs = runs;
    Salesmen = 1;
    Scale = -1;
    Seed = seed;
    SierpinskiPartitioning = 0;
    StopAtOptimum = 1;
    Subgradient = 1;
    SubproblemBorders = 0;
    SubproblemsCompressed = 0;
    SubproblemSize = 0;
    SubsequentMoveType = 0;
    SubsequentMoveTypeSpecial = 0;
    SubsequentPatching = 1;
    TimeLimit = time_limit > 0.0 ? time_limit : 1e308;
    TotalTimeLimit = time_limit > 0.0 ? time_limit : 1e308;
    TraceLevel = 0;
    TSPTW_Makespan = 0;
    Norm = 9999;
    Asymmetric = 0;
    Penalty = nullptr;

    ParameterFileName = nullptr;
    ProblemFileName = nullptr;
    PiFileName = nullptr;
    InputTourFileName = nullptr;
    OutputTourFileName = nullptr;
    TourFileName = nullptr;
    CandidateFileName = nullptr;
    EdgeFileName = nullptr;
    CandidateFiles = 0;
    EdgeFiles = 0;
    MergeTourFiles = 0;
    MergeTourFileName = nullptr;

    CostMatrix = nullptr;
    Distance = Distance_EXPLICIT;
    D = D_EXPLICIT;
    C = C_EXPLICIT;
    c = nullptr;
}

static void setupNodes(int dimension)
{
    local_NodeSet = (Node *)calloc(dimension + 1, sizeof(Node));
    if (!local_NodeSet) {
        throw std::runtime_error("Failed to allocate NodeSet");
    }

    NodeSet = local_NodeSet;

    for (int i = 1; i <= dimension; i++) {
        Node *N = &NodeSet[i];
        N->Id = i;
        N->OriginalId = i;
        N->Pi = 0;
        N->V = 0;
        N->CandidateSet = nullptr;
        N->BackboneCandidateSet = nullptr;
        N->FixedTo1 = nullptr;
        N->FixedTo2 = nullptr;
        N->BestSuc = nullptr;
        N->NextBestSuc = nullptr;
        N->InitialSuc = nullptr;
        N->Next = nullptr;
        N->Dad = nullptr;
        N->Mark = nullptr;
        N->Nearest = nullptr;
        N->Head = nullptr;
        N->Tail = nullptr;
        N->InputSuc = nullptr;
        N->SubproblemPred = nullptr;
        N->SubproblemSuc = nullptr;
        N->SubBestPred = nullptr;
        N->SubBestSuc = nullptr;
        N->MergePred = nullptr;
        N->MergeSuc = nullptr;
        N->Added1 = nullptr;
        N->Added2 = nullptr;
        N->Deleted1 = nullptr;
        N->Deleted2 = nullptr;
        N->SucSaved = nullptr;
        N->Parent = nullptr;
        N->FirstConstraint = nullptr;
        N->PathLength = nullptr;
        N->Path = nullptr;
        N->ColorAllowed = nullptr;
        N->ServiceTime = 0.0;
        N->Pickup = 0;
        N->Delivery = 0;
        N->DepotId = 0;
        N->Earliest = 0.0;
        N->Latest = 0.0;
        N->Backhaul = 0;
        N->Serial = 0;
        N->Color = 0;
        N->Group = 0;
        N->X = 0.0;
        N->Y = 0.0;
        N->Z = 0.0;
        N->Xc = 0.0;
        N->Yc = 0.0;
        N->Zc = 0.0;
        N->Axis = 0;
        N->Score = 0;
        N->OldPredExcluded = 0;
        N->OldSucExcluded = 0;
        N->Required = 1;
        N->Demand = 0;
        N->M_Demand = nullptr;
        N->Seq = 0;
        N->DraftLimit = 0;
        N->Load = 0;
        N->Special = 0;
        N->Loc = 0;
        N->Rank = 0;
        N->Cost = 0;
        N->NextCost = 0;
        N->PredCost = 0;
        N->SucCost = 0;
        N->SavedCost = 0;
        N->Beta = 0;
        N->Subproblem = 0;
        N->Sons = 0;
        N->C = nullptr;
        N->OldPred = nullptr;
        N->OldSuc = nullptr;
    }

    for (int i = 1; i <= dimension; i++) {
        LKH_Link(&NodeSet[i], &NodeSet[i + 1 <= dimension ? i + 1 : 1]);
    }

    local_FirstNode = &NodeSet[1];
    FirstNode = local_FirstNode;
}

static void setupCostMatrix(const DistMatrix& dist, int dimension)
{
    size_t matrix_size = (size_t)dimension * (dimension - 1) / 2;
    local_CostMatrix = (int *)calloc(matrix_size, sizeof(int));
    if (!local_CostMatrix) {
        throw std::runtime_error("Failed to allocate CostMatrix");
    }

    CostMatrix = local_CostMatrix;

    for (int i = 1; i <= dimension; i++) {
        Node *Ni = &NodeSet[i];
        Ni->C = &CostMatrix[(size_t)(i - 1) * (i - 2) / 2] - 1;
        for (int j = 1; j < i; j++) {
            Ni->C[j] = dist[i - 1][j - 1];
        }
    }
}

static void setupMoveFunctions()
{
    static MoveFunction BestOptMove[] = {
        nullptr, nullptr, Best2OptMove, Best3OptMove, Best4OptMove, Best5OptMove
    };

    int K = MoveType >= SubsequentMoveType || !SubsequentPatching ?
        MoveType : SubsequentMoveType;

    if (PatchingC >= 1) {
        BestMove = BestSubsequentMove = BestKOptMove;
        if (!SubsequentPatching && SubsequentMoveType <= 5) {
            BestSubsequentMove = BestOptMove[SubsequentMoveType];
        }
    } else {
        BestMove = (MoveType <= 5) ? BestOptMove[MoveType] : BestKOptMove;
        BestSubsequentMove = (SubsequentMoveType <= 5) ?
            BestOptMove[SubsequentMoveType] : BestKOptMove;
    }
    if (MoveTypeSpecial) BestMove = BestSpecialOptMove;
    if (SubsequentMoveTypeSpecial) BestSubsequentMove = BestSpecialOptMove;

    MergeWithTour = MergeWithTourIPT;
}

static void computeDerivedParams()
{
    if (SubsequentMoveType == 0) SubsequentMoveType = MoveType;
    if (MaxSwaps < 0) MaxSwaps = Dimension;
    if (MaxTrials < 0) MaxTrials = Dimension;
    if (Excess < 0) Excess = 1.0 / DimensionSaved;
    if (InitialPeriod < 0) InitialPeriod = (int)fmax(Dimension / 2, 100.0);
    if (InitialStepSize == 0) InitialStepSize = 1;
    if (Scale < 0) Scale = 1;
    if (NonsequentialMoveType < 0)
        NonsequentialMoveType = MoveType + PatchingC + PatchingA - 1;
}

static void cleanupResources()
{
    FreeStructures();
    local_CostMatrix = nullptr;
    CostMatrix = nullptr;
    local_NodeSet = nullptr;
    NodeSet = nullptr;
    local_FirstNode = nullptr;
    FirstNode = nullptr;
}

long long lkh_solve(const DistMatrix& dist,
                                                    int tour[],
                                                    int max_trials,
                                                    int max_candidates,
                                                    int runs,
                                                    int move_type,
                                                    unsigned seed,
                                                    double time_limit)
{
    int n = (int)dist.size();
    if (n <= 1) {
        return 0;
    }
    if (n == 2) {
        return (long long)dist[0][1] + dist[1][0];
    }

    try {
        setParameterDefaults(n, max_trials, max_candidates, runs,
                              move_type, seed, time_limit);
        setupNodes(n);
        setupCostMatrix(dist, n);
        computeDerivedParams();
        setupMoveFunctions();

        AllocateStructures();
        CreateCandidateSet();
        InitializeStatistics();

        BestCost = PLUS_INFINITY;
        BestPenalty = CurrentPenalty = PLUS_INFINITY;

        double LastTime = GetTime();
        StartTime = LastTime;

        for (Run = 1; Run <= Runs; Run++) {
            LastTime = GetTime();
            if (LastTime - StartTime >= TotalTimeLimit) {
                Run--;
                break;
            }

            GainType Cost = FindTour();

            if (CurrentPenalty < BestPenalty ||
                (CurrentPenalty == BestPenalty && Cost < BestCost)) {
                BestPenalty = CurrentPenalty;
                BestCost = Cost;
                RecordBetterTour();
                RecordBestTour();
            }

            if (StopAtOptimum && BestCost == Optimum) {
                break;
            }

            SRandom(++Seed);
        }

        long long cost = -1;

        if (BestCost != PLUS_INFINITY && BestTour[1] != 0) {
            for (int i = 0; i < n; i++) {
                tour[i] = BestTour[i + 1] - 1;
            }
            cost = (long long)BestCost;
        } else {
            for (int i = 0; i < n; i++) tour[i] = i;
            cost = 0;
            for (int i = 0; i < n; i++) {
                cost += dist[i][(i + 1) % n];
            }
        }

        cleanupResources();
        return cost;

    } catch (...) {
        cleanupResources();
        for (int i = 0; i < n; i++) tour[i] = i;
        long long cost = 0;
        for (int i = 0; i < n; i++) {
            cost += dist[i][(i + 1) % n];
        }
        return cost;
    }
}
