#include <algorithm>
#include <iostream>
#include <iterator>
#include <numeric>
#include <vector>
#include <utility>
#include <string>

using std::cin;
using std::copy;
using std::cout;
using std::distance;
using std::endl;
using std::iterator_traits;
using std::less;
using std::next;
using std::ostream;
using std::prev;
using std::string;
using std::vector;

// Represents a player with some efficiency and id.
struct Player {
    long long efficiency;
    unsigned int id;
};

typedef vector<Player>::iterator PlayerIterator;

bool operator == (const Player& left, const Player& right) {
    return left.efficiency == right.efficiency && left.id == right.id;
}

// Compares two players by their efficiency.
// If efficiencies are equal, compares by ids. 
bool CompareByEfficiency(const Player& first, const Player& second) {
    if (first.efficiency != second.efficiency) {
        return first.efficiency < second.efficiency;
    } else {
        return first.id < second.id;
    }
}

// Compare two players by their ids.
// If ids are equal, compares by efficiencies. 
bool CompareById(const Player& first, const Player& second) {
    if (first.id != second.id) {
        return first.id < second.id;
    } else {
        return first.efficiency < second.efficiency;
    }
}

// Represents an interval of players of the team.
template <class RandomAccessIterator>
class TeamInterval {
public:
    TeamInterval(RandomAccessIterator first
               , RandomAccessIterator last) : first_(first), last_(last) {
        efficiency_ = calculate_efficiency_();
    }

    RandomAccessIterator first() const {
        return first_;
    }

    RandomAccessIterator last() const {
        return last_;
    }

    RandomAccessIterator second() const {
        return first_ + 1;
    }

    RandomAccessIterator end() const {
        return next(last_);
    }

    long long efficiency() const {
        return efficiency_;
    }

    void shift_first() {
        efficiency_ -= first_->efficiency;
        ++first_;        
    }

    void shift_last() {
        ++last_;
        efficiency_ += last_->efficiency;
    }

private:
    long long calculate_efficiency_() {
        long long efficiency = 0;
        for (auto it = first_; it != end(); ++it) {
            efficiency += it->efficiency;
        }
        return efficiency;
    }

    RandomAccessIterator first_, last_;
    long long efficiency_;
};

// Sorts elements in interval [bwgin, end),
// the array elements compare with `comp`. 
template <typename Iterator, typename Comparator>
void Sort(Iterator begin, Iterator end, Comparator cmp) {
    size_t length = distance(begin, end);
    if (length < 2) {
        return;
    }

    size_t middle = length / 2;
    Iterator pivot = next(begin, middle);

    Sort(begin, pivot, cmp);
    Sort(pivot, end, cmp);
    Merge(begin, pivot, end, cmp);
}

template <typename Iterator, typename Comparator>
void Merge(Iterator begin, Iterator pivot, Iterator end, Comparator cmp) {
    vector<typename iterator_traits<Iterator>::value_type> firstPart(begin, pivot);
    vector<typename iterator_traits<Iterator>::value_type> secondPart(pivot, end);

    Iterator firstIter = firstPart.begin();
    Iterator secondIter = secondPart.begin();

    while (firstIter != firstPart.end()) {
        if (secondIter == secondPart.end()) {
            copy(firstIter, firstPart.end(), begin);
            return;
        }
        if (cmp(*secondIter, *firstIter)) {
            *begin = *secondIter++;
        } else {
            *begin = *firstIter++;
        }
        ++begin;
    }
    copy(secondIter, secondPart.end(), begin);
}

// Represents a team.
// Creates from input array of players a team with the highest efficiency. 
class Team {
public:
    Team() : efficiency_(0) {}

    // Constructs a team from players. The team will be ordered by players id. 
    explicit Team(const TeamInterval<PlayerIterator>& playersInterval) 
        : players_(playersInterval.first(), playersInterval.end()),
          efficiency_(playersInterval.efficiency()) {
            
        Sort(players_.begin(), players_.end(), CompareById);
    }

    // Prints to stream two lines.
    // The first is total efficiency of the team.
    // The second is the players numbers splited by spaces.
    friend ostream& operator << (ostream& stream, Team& team) {
        cout << team.efficiency_ << endl;
        for (int i = 0; i < team.players_.size(); ++i) {
            cout << team.players_[i].id << ' ';
        }

        return stream;
    }

    long long efficiency() const {
        return efficiency_;
    }

    vector<int> playersIds() const {
        vector<int> ids(players_.size());
        for (int i = 0; i < players_.size(); ++i) {
            ids[i] = players_[i].id;
        }
        return ids;
    }

private:
    long long efficiency_;
    vector<Player> players_;
};

// Builds from `players` the team with the highest efficiency.
Team buildMaxEfficiencyTeam(vector<Player> players) {
    Sort(players.begin(), players.end(), CompareByEfficiency);

    TeamInterval<PlayerIterator> currentInterval(players.begin(), players.begin());
    TeamInterval<PlayerIterator> bestInterval(players.begin(), players.begin());

    while (currentInterval.last() != prev(players.end())) {
        currentInterval.shift_last();

        while (currentInterval.first()->efficiency + currentInterval.second()->efficiency
                < currentInterval.last()->efficiency) {
            currentInterval.shift_first();
        }

        if (currentInterval.efficiency() > bestInterval.efficiency()) {
            bestInterval = currentInterval;
        }
    }

    Team maxEfficiencyTeam(bestInterval);
    return maxEfficiencyTeam;
}

// Reads from standard input stream the number of the players and their efficiencies.
vector<Player> ReadInput() {
    int playersCount;
    cin >> playersCount;

    vector<Player> players(playersCount);
    for (int i = 0; i < playersCount; ++i) {
        int playerEfficiency;
        cin >> playerEfficiency;
        players[i].efficiency = playerEfficiency;
        players[i].id = i + 1;
    }

    return players;
}

// Launches all tests.
void TestAll();

int main(int argc, char *argv[]) {
    if (argc == 2 && std::string(argv[1]) == "--test") {
        TestAll();
    } else {
        vector<Player> players;
        players = ReadInput();
        Team idealTeam = buildMaxEfficiencyTeam(players);
        cout << idealTeam;
    }
    
    return 0;
}

template<typename T>
ostream& operator << (ostream& stream, const vector<T>& vec) {
    stream << "{";
    for (size_t i = 0; i < vec.size(); ++i) {
        stream << vec[i];
        if (i + 1 < vec.size()) {
            stream << ",";
        }
    }
    return stream << "}";
}

ostream& operator << (ostream& stream, const Player& player) {
    stream << '(' << player.efficiency << ',' << player.id << ')';
    return stream;
}

// Compares calculated value `result` with the right value `expected`.
// If result differ from expected, throws RuntimeError
template<typename T, typename U, typename V>
void CheckResult(T input, U result, V expected, string functionName) {
    if (!(result == expected)) {
        std::cerr << "While testing " << functionName << ". "
            << "For input " << input
            << " expected " << expected
            << " but got "  << result << endl;
        throw std::runtime_error("Test failed");
    }
}

// Launches the `Sort` function after that executes `CheckResult`.
template<typename T, class Compare>
void TestSort(vector<T> input, vector<T> expected, Compare comp) {
    vector<T> result = input;
    Sort(result.begin(), result.end(), comp);
    CheckResult(input, result, expected, "Sort");
}

// Returns a random number from the range.
int Random(int rangeMin, int rangeMax) {
    int rangeSize = rangeMax - rangeMin + 1;
    return rangeMin + rand() % rangeSize;
}

// Tests the `Sort` at arrays of random numbers.
// The numbers generated by the `Random` function.
void StressTestSortNumbers(int maxLength, int maxItemAbs) {
    vector<int> input(Random(1, maxLength));
    for (size_t i = 0; i < input.size(); ++i) {
        input[i] = Random(-maxItemAbs, maxItemAbs);
    }

    vector<int> answer = input;
    std::sort(answer.begin(), answer.end());
    vector<int> result = input;
    Sort(result.begin(), result.end(), less<>());

    CheckResult(input, result, answer, "Sort");
}

// Tests the `Sort` at arrays of random Players.
// The Players generated by the `Random` function.
template<class Compare>
void StressTestSortPlayers(int maxLength, int maxItem, Compare cmp) {
    vector<Player> input(Random(1, maxLength));
    for (size_t i = 0; i < input.size(); ++i) {
        input[i] = Player{ Random(0, maxItem), Random(0, maxItem) };
    }

    vector<Player> answer = input;
    std::sort(answer.begin(), answer.end(), cmp);
    vector<Player> result = input;
    Sort(result.begin(), result.end(), cmp);

    CheckResult(input, result, answer, "Sort");
}

// Launches the `TestSort` function with different input arrays of numbers.
void TestSortNumbers() {
    TestSort(vector<int>{ 4, 1, 3 }, vector<int>{ 1, 3, 4 }, less<>());
    TestSort(vector<int>{ 3, 2, 1, 3, 2 }, vector<int>{ 1, 2, 2, 3, 3 }, less<>());
    TestSort(vector<int>{ -1, 3, -5, 6, 2 }, vector<int>{ -5, -1, 2, 3, 6 }, less<>());

    // corner cases
    TestSort(vector<int>{ 1, 2, 3, 4, 5 }, vector<int>{ 1, 2, 3, 4, 5 }, less<>());
    TestSort(vector<int>{ -1, -2, -3, -4, -5 }, vector<int>{ -5, -4, -3, -2, -1 }, less<>());
    TestSort(vector<int>{ 4, 4, 4, 4 }, vector<int>{ 4, 4, 4, 4 }, less<>());
    TestSort(vector<int>{ 10 }, vector<int>{ 10 }, less<>());
    TestSort(vector<int>{}, vector<int>{}, less<>());
    TestSort(vector<int>(100, 5), vector<int>(100, 5), less<>());

    srand(21102014);
    const size_t smallTestCount = 1000;
    for (size_t testNum = 1; testNum <= smallTestCount; ++testNum) {
        cout << "Test " << testNum << endl;
        StressTestSortNumbers(10, 10);
    }
    const size_t bigTestCount = 1000000;
    for (size_t testNum = smallTestCount + 1; testNum <= bigTestCount; ++testNum) {
        cout << "Test " << testNum << endl;
        StressTestSortNumbers(100, 1000);
    }
}

// Launches the `TestSort` function with different input Players.
void TestSortPlayers() {
    typedef Player P;
    TestSort(vector<P>{ P{ 4, 1 }, P{ 1, 5 }, P{ 3, 2 } },
             vector<P>{ P{ 1, 5 }, P{ 3, 2 }, P{ 4, 1 } },
             CompareByEfficiency);
    TestSort(vector<P>{ P{ 4, 1 }, P{ 1, 5 }, P{ 3, 2 } },
             vector<P>{ P{ 4, 1 }, P{ 3, 2 }, P{ 1, 5 } },
             CompareById);
    TestSort(vector<P>{ P{ 3, 9 }, P{ 2, 4 }, P{ 1, 7 }, P{ 3, 1 }, P{ 1, 9 } },
             vector<P>{ P{ 1, 7 }, P{ 1, 9 }, P{ 2, 4 }, P{ 3, 1 }, P{ 3, 9 } },
             CompareByEfficiency);
    TestSort(vector<P>{ P{ 3, 9 }, P{ 2, 4 }, P{ 1, 7 }, P{ 3, 1 }, P{ 1, 9 } },
             vector<P>{ P{ 3, 1 }, P{ 2, 4 }, P{ 1, 7 }, P{ 1, 9 }, P{ 3, 9 } },
             CompareById);
    TestSort(vector<P>{ P{ 2, 5 }, P{ 5, 7 }, P{ 8, 2 }, P{ 1, 6 }, P{ 4, 9 } },
             vector<P>{ P{ 1, 6 }, P{ 2, 5 }, P{ 4, 9 }, P{ 5, 7 }, P{ 8, 2 } },
             CompareByEfficiency);
    TestSort(vector<P>{ P{ 2, 5 }, P{ 5, 7 }, P{ 8, 2 }, P{ 1, 6 }, P{ 4, 9 } },
             vector<P>{ P{ 8, 2 }, P{ 2, 5 }, P{ 1, 6 }, P{ 5, 7 }, P{ 4, 9 } },
             CompareById);

    // corner cases
    TestSort(vector<P>{ P{ 1, 2 }, P{ 2, 9 }, P{ 3, 8 }, P{ 4, 6 }, P{ 5, 8 } },
             vector<P>{ P{ 1, 2 }, P{ 2, 9 }, P{ 3, 8 }, P{ 4, 6 }, P{ 5, 8 } },
             CompareByEfficiency);
    TestSort(vector<P>{ P{ 6, 1 }, P{ 2, 2 }, P{ 4, 3 }, P{ 3, 4 }, P{ 9, 5 } },
             vector<P>{ P{ 6, 1 }, P{ 2, 2 }, P{ 4, 3 }, P{ 3, 4 }, P{ 9, 5 } },
             CompareById);
    TestSort(vector<P>{ P{ 4, 4 }, P{ 4, 4 }, P{ 4, 4 }, P{ 4, 4 } },
             vector<P>{ P{ 4, 4 }, P{ 4, 4 }, P{ 4, 4 }, P{ 4, 4 } },
             CompareByEfficiency);
    TestSort(vector<P>{ P{ 4, 4 }, P{ 4, 4 }, P{ 4, 4 }, P{ 4, 4 } },
             vector<P>{ P{ 4, 4 }, P{ 4, 4 }, P{ 4, 4 }, P{ 4, 4 } },
             CompareById);
    TestSort(vector<P>{ P{ 10, 10 } },
             vector<P>{ P{ 10, 10 } },
             CompareByEfficiency);
    TestSort(vector<P>{ P{ 10, 10 } },
             vector<P>{ P{ 10, 10 } },
             CompareById);
    TestSort(vector<P>{ }, 
             vector<P>{ }, 
             CompareByEfficiency);
    TestSort(vector<P>{ }, 
             vector<P>{ }, 
             CompareById);

    srand(21102014);
    const size_t smallTestCount = 1000;
    for (size_t testNum = 1; testNum <= smallTestCount; ++testNum) {
        cout << "Test " << testNum << endl;
        StressTestSortPlayers(10, 10, CompareByEfficiency);
        StressTestSortPlayers(10, 10, CompareById);
    }
    const size_t bigTestCount = 1000000;
    for (size_t testNum = smallTestCount + 1; testNum <= bigTestCount; ++testNum) {
        cout << "Test " << testNum << endl;
        StressTestSortPlayers(100, 1000, CompareByEfficiency);
        StressTestSortPlayers(100, 1000, CompareById);
    }
}

// Launches tests of `Sort` function on different data types.
void TestSortAll() {
    cout << "Testing Sort with array of numbers: " << endl;
    TestSortNumbers();

    cout << "Testing Sort with array of Players: " << endl;
    TestSortPlayers();
}

// Tests the `createTeam` function of the `Team` class.
void TestTeamCreate(vector<int> input, vector<int> expected) {
    vector<Player> players(input.size());
    for (int i = 0; i < input.size(); ++i) {
        players[i] = Player{ input[i], i + 1 };
    }

    Team idealTeam = buildMaxEfficiencyTeam(players);
    auto result = idealTeam.playersIds();
    CheckResult(input, result, expected, "createTeam");
}

// Launches the `TestTeamCreate` function with different input.
void TestTeamCreateAll() {
    cout << "Testing createTeam method: " << endl;
    TestTeamCreate({ 3, 2, 5, 4, 1 }, { 1, 2, 3, 4 });
    TestTeamCreate({ 1, 2, 4, 8, 16 }, { 4, 5 });
    TestTeamCreate({ 1, 5, 2, 3, 4, 9, 6, 2, 1, 3 }, { 2, 5, 6, 7 });
    TestTeamCreate({ 5, 5, 5, 5, 5 }, { 1, 2, 3, 4, 5 });
    TestTeamCreate({ 1 }, { 1 });
}

void TestAll() {
    TestSortAll();
    TestTeamCreateAll();
}
