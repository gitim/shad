#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <iterator>
#include <list>
#include <string>
#include <vector>
#include <utility>

using std::cerr;
using std::cin;
using std::cout;
using std::endl;
using std::greater;
using std::istream;
using std::iterator_traits;
using std::list;
using std::make_heap;
using std::next;
using std::ostream;
using std::string;
using std::runtime_error;
using std::swap;
using std::vector;

const int OUT_OF_HEAP = -1;
const int FAIL_CODE = -1;

enum MemoryState {
    IS_FREE,
    IS_OCCUPIED
};

enum RequestType {
    ALLOCATION = 1,
    REVOCATION = 2
};       

struct MemoryPart {
    int size;
    int offset;
    int index;
    MemoryState state;

    MemoryPart()
        : index(OUT_OF_HEAP),
          state(IS_FREE)
    {}

    MemoryPart(int size, int offset, int index, MemoryState state)
        : size(size),
          offset(offset),
          index(index),
          state(state)
    {}
};

typedef list<MemoryPart>::iterator MemoryPartIterator;

class CompareMemoryPartsBySize {
public:
    bool operator() (const MemoryPartIterator& left, const MemoryPartIterator& right) const {
        if (left->size == right->size) {
            return left->offset < right->offset;
        }
        return left->size > right->size;
    }
};

void SwapMemoryPartIterators(MemoryPartIterator& first, MemoryPartIterator& second) {
    if (first->index == second->index) {
        return;
    }

    swap(first->index, second->index);
    swap(first, second);
}

class EmptyHeapException : public runtime_error {
public:
    EmptyHeapException()
        : runtime_error("attempted to pop from empty heap")
    {}
};

class MethodNotOverridenException : public runtime_error {
public:
    MethodNotOverridenException()
        : runtime_error("the method must be overriden")
    {}
};

class UnknownRequestTypeException : public runtime_error {
public:
    UnknownRequestTypeException()
        : runtime_error("there is no suitable method for this type of request")
    {}
};

template <
    typename T,
    class Compare = greater<T>,
    void(*Swap)(T& first, T& second) = swap
>
class Heap {
    friend void CheckHeap(const vector<int>& input, const Heap<int>& heap, string methodName);

public:
    Heap() {}

    explicit Heap(const vector<T>& elements) 
        : elements_(elements) 
    {
        for (int i = elements_.size() / 2; i >= 0; --i) {
            int position = i;
            siftDown(position);
        }
    }

    bool empty() const {
        return elements_.empty();
    }

    void remove(int position) {
        if (!empty()) {
            if (position >= size() || position < 0) {
                return;
            }

            if (position == size() - 1) {
                elements_.pop_back();
                return;
            }

            Swap(elements_.at(position), elements_.at(size() - 1));
            elements_.pop_back();

            if (!empty()) {
                updateHeap(position);
            }
        }
    }

    size_t size() const {
        return elements_.size();
    }

    const T& top() const {
        if (!empty()) {
            return elements_.front();
        } else {
            throw EmptyHeapException();
        }
    }

    void pop() {
        if (!empty()) {
            remove(0);
        } else {
            throw EmptyHeapException();
        }
    }

    void insert(const T& elem) {
        elements_.push_back(elem);
        updateHeap(size() - 1);
    }

private:
    // Restores heap features.
    void updateHeap(int position) {
        siftUp(position);
        siftDown(position);
    }

    void siftUp(int& position) {
        int parent = (position - 1) / 2;

        while (compare_(elements_.at(position), elements_.at(parent))) {
            Swap(elements_.at(position), elements_.at(parent));
            position = parent;
            parent = (position - 1) / 2;
        }
    }

    void siftDown(int& position) {
        for (int iter = elements_.size() / 2; iter >= 1; --iter) {
            int leftChild = position * 2 + 1;
            int rightChild = position * 2 + 2;
            int largestChild = position;

            if (leftChild < size() &&
                compare_(elements_.at(leftChild), elements_.at(largestChild))) {
                largestChild = leftChild;
            }
            if (rightChild < size() &&
                compare_(elements_.at(rightChild), elements_.at(largestChild))) {
                largestChild = rightChild;
            }
            if (largestChild == position) {
                break;
            }

            Swap(elements_.at(position), elements_.at(largestChild));
            position = largestChild;
        }
    }

    Compare compare_;
    vector<T> elements_;
};

class Operation {
public:
    unsigned int id;
    MemoryPartIterator part;
    
    Operation(unsigned int id, const MemoryPartIterator& part)
        : id(id),
          part(part)
    {}
};

typedef vector<Operation>::iterator OperationIterator;

class MemoryManager {
public:
    explicit MemoryManager(int memorySize) 
        : requestsCount(0) 
    {
        MemoryPart allMemory(memorySize, 1, 0, IS_FREE);
        memoryParts_.push_back(allMemory);
        freeMemory_.insert(memoryParts_.begin());
    }

    // Revokes the request for memory allocating.
    // Takes the number of request for revoking.
    void revoke(size_t requestNumber) {
        ++requestsCount;
        OperationIterator operationForRevoke = findOperationById(requestNumber);

        if (operationForRevoke == operationsHistory_.end()) {
            return;
        }

        MemoryPartIterator part = operationForRevoke->part;
        part->state = IS_FREE;
        merge(part, next(part));

        MemoryPartIterator prev = part;
        if (prev->offset != 1) {
            --prev;
        }

        bool result = merge(prev, part);
        if (result) {
            prev->index = freeMemory_.size();
            freeMemory_.insert(prev);
        } else {
            part->index = freeMemory_.size();
            freeMemory_.insert(part);
        }

        operationsHistory_.erase(operationForRevoke);
    }

    // Tries to allocate memory of the given size.
    // Returns the offset of the allocated memory part in success,
    // else returns -1.
    int allocate(int requestedMemorySize) {
        ++requestsCount;
        if (freeMemory_.empty()) {
            return FAIL_CODE;
        }

        MemoryPartIterator maxSizeMemoryPart = freeMemory_.top();

        if (maxSizeMemoryPart->size < requestedMemorySize) {
            return FAIL_CODE;
        }
        freeMemory_.pop();

        int firstFreeCell = maxSizeMemoryPart->offset;

        if (maxSizeMemoryPart->size > requestedMemorySize) {
            MemoryPart newMemoryPart(requestedMemorySize,
                                     maxSizeMemoryPart->offset,
                                     OUT_OF_HEAP,
                                     IS_OCCUPIED);
            maxSizeMemoryPart->offset += requestedMemorySize;
            maxSizeMemoryPart->size -= requestedMemorySize;
            maxSizeMemoryPart->index = freeMemory_.size();

            memoryParts_.insert(maxSizeMemoryPart, newMemoryPart);
            freeMemory_.insert(maxSizeMemoryPart);
            --maxSizeMemoryPart;
        } else {
            maxSizeMemoryPart->state = IS_OCCUPIED;
        }

        Operation operation(requestsCount, maxSizeMemoryPart);
        operationsHistory_.push_back(operation);
        return firstFreeCell;
    }   

private:
    // Tries to merge two memory parts.
    // Returns `true` if the parts were merged, else returns `false`.
    bool merge(MemoryPartIterator first, MemoryPartIterator second) {
        if (second == memoryParts_.end() || second == memoryParts_.begin()) {
            return false;
        }

        if (first->state == IS_FREE && second->state == IS_FREE) {
            freeMemory_.remove(first->index);
            freeMemory_.remove(second->index);
            first->index = OUT_OF_HEAP;
            second->index = OUT_OF_HEAP;
            first->size += second->size;
            memoryParts_.erase(second);
            return true;
        }

        return false;
    }

    OperationIterator findOperationById(unsigned int id) {
        int left = 0;
        int right = operationsHistory_.size() - 1;

        while (left <= right) {
            int mid = left + (right - left) / 2;

            if (operationsHistory_.at(mid).id == id) {
                return operationsHistory_.begin() + mid;
            }
            if (operationsHistory_.at(mid).id > id) {
                right = mid - 1;
            } else {
                left = mid + 1;
            }
        }
         
        return operationsHistory_.end();
    }

    vector<Operation> operationsHistory_;
    list<MemoryPart> memoryParts_;
    Heap<MemoryPartIterator,
        CompareMemoryPartsBySize,
        SwapMemoryPartIterators> freeMemory_;
    unsigned int requestsCount;
};

class Request {
    friend Request* readInput(istream& stream);

public:         
    Request() {}

    virtual ~Request() {}

    RequestType type() const {
        return type_;
    }

    virtual int size() const {
        throw MethodNotOverridenException();
    }

    virtual int operationForRevoke() const {
        throw MethodNotOverridenException();
    }

protected:
    RequestType type_;
};

class AllocationRequest : public Request {
    friend Request* readInput(istream& stream);

public:
    explicit AllocationRequest(int size)
        : size_(size)
    {
        type_ = ALLOCATION;
    }

    int size() const {
        return size_;
    }

private:
    int size_;
};

class RevocationRequest : public Request {
    Request* readInput(istream& stream);

public:
    explicit RevocationRequest(int operation)
        : operationForRevoke_(operation)
    {
        type_ = REVOCATION;
    }

    int operationForRevoke() const {
        return operationForRevoke_;
    }

private:
    int operationForRevoke_;
};

Request* readInput(istream& stream) {
    int rawRequest;
    stream >> rawRequest;

    if (rawRequest >= 0) {
        return new AllocationRequest(rawRequest);
    } else {
        return new RevocationRequest(-rawRequest);
    }
}

int processRequest(MemoryManager& memoryManager, const Request& request) {
    switch (request.type()) {
    case ALLOCATION:
        return memoryManager.allocate(request.size());
        break;
    case REVOCATION:
        memoryManager.revoke(request.operationForRevoke());
        break;
    default:
        throw UnknownRequestTypeException();
    }
}

void TestAll();

int main(int argc, char *argv[]) {  
    if (argc == 2 && std::string(argv[1]) == "--test") {
        TestAll();
    } else {
        int memorySize;
        int requestNumber;
        cin >> memorySize >> requestNumber;
        MemoryManager memoryManager(memorySize);

        for (int i = 0; i < requestNumber; ++i) {
            Request* request = readInput(cin);
            int result = processRequest(memoryManager, *request);
            if (request->type() == ALLOCATION) {
                cout << result << endl;
            }
            delete request;
        }
    }
    return 0;
}

//############################Testing################################

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

// Compares calculated value `result` with the right value `expected`.
// If result differ from expected, throws RuntimeError
template<typename T, typename U, typename V>
void CheckResult(T input, U result, V expected, string functionName) {
    if (!(result == expected)) {
        cerr << "While testing " << functionName << ". "
             << "For input "     << input
             << " expected "     << expected
             << " but got "      << result       << endl;
        throw runtime_error("Test failed");
    }
}
        
void CheckHeap(const vector<int>& input, const Heap<int>& heap, string methodName) {
    // If heap is correct make_heap won't change `answer`.
    vector<int> answer = heap.elements_;
    make_heap(answer.begin(), answer.end());
    CheckResult(input, heap.elements_, answer, methodName + " of the Heap");
}

// Returns a random number from the range.
int Random(int rangeMin, int rangeMax) {
    int rangeSize = rangeMax - rangeMin + 1;
    return rangeMin + rand() % rangeSize;
}

vector<int> RandomVector(int maxLength, int maxItemAbs) {
    vector<int> randomVector(Random(1, maxLength));
    for (size_t i = 0; i < randomVector.size(); ++i) {
        randomVector[i] = Random(-maxItemAbs, maxItemAbs);
    }
    return randomVector;
}

void TestHeapConstructor(const vector<int>& input) {
    Heap<int> heap(input);
    CheckHeap(input, heap, "single parameter constructor");
}

void StressTestConstructor(int maxLength, int maxItemAbs) {
    vector<int> input = RandomVector(maxLength, maxItemAbs);
    TestHeapConstructor(input);
}

void TestHeapConstructorAll() {
    TestHeapConstructor(vector<int>{1, 2, 3, 4, 5});
    TestHeapConstructor(vector<int>{5, 4, 3, 2, 1});
    TestHeapConstructor(vector<int>{3, 5, 2, 1, 4});
    TestHeapConstructor(vector<int>{5, 5, 5, 5, 5});
    TestHeapConstructor(vector<int>{10});

    srand(07012014);
    const size_t smallTestCount = 1000;
    for (size_t testNum = 1; testNum <= smallTestCount; ++testNum) {
        cout << "Test " << testNum << endl;
        StressTestConstructor(10, 10);
    }

    const size_t bigTestCount = 1000;
    for (size_t testNum = smallTestCount + 1; testNum <= bigTestCount; ++testNum) {
        cout << "Test " << testNum << endl;
        StressTestConstructor(100, 1000);
    }
}

void TestHeapInsert(const vector<int>& currentHeap, int newElement) {
    Heap<int> heap(currentHeap);
    heap.insert(newElement);
    CheckHeap(currentHeap, heap, "insert");
}

void StressTestInsert(int maxLength, int maxItemAbs, int maxInsertionsCount) {
    vector<int> input = RandomVector(maxLength, maxItemAbs);

    int insertionsCount = Random(0, maxInsertionsCount);
    for (size_t testNum = 1; testNum <= insertionsCount; ++testNum) {
        int newElement = Random(-maxItemAbs, maxItemAbs);
        TestHeapInsert(input, newElement);
    }
}

void TestHeapInsertAll() {
    TestHeapInsert(vector<int>{1, 2, 3, 4, 5}, 6);
    TestHeapInsert(vector<int>{5, 4, 3, 2, 1}, 0);
    TestHeapInsert(vector<int>{3, 8, 2, 1, 4}, 5);
    TestHeapInsert(vector<int>{5, 5, 5, 5, 5}, 5);

    srand(07012014);
    const size_t smallTestCount = 1000;
    for (size_t testNum = 1; testNum <= smallTestCount; ++testNum) {
        cout << "Test " << testNum << endl;
        StressTestInsert(10, 10, 1);
    }

    const size_t bigTestCount = 1000;
    for (size_t testNum = smallTestCount + 1; testNum <= bigTestCount; ++testNum) {
        cout << "Test " << testNum << endl;
        StressTestInsert(100, 1000, 10);
    }
}

void TestHeapRemove(const vector<int>& currentHeap, int positionForRemove) {
    Heap<int> heap(currentHeap);
    heap.remove(positionForRemove);
    CheckHeap(currentHeap, heap, "remove");
}

void StressTestRemove(int maxLength, int maxItemAbs, int maxRemovalCount) {
    vector<int> input = RandomVector(maxLength, maxItemAbs);

    int insertionsCount = Random(0, maxRemovalCount);
    for (size_t testNum = 1; testNum <= insertionsCount; ++testNum) {
        int position = Random(0, maxItemAbs);
        TestHeapRemove(input, position);
    }
}

void TestHeapRemoveAll() {
    TestHeapRemove(vector<int>{1, 2, 3, 4, 5}, 3);
    TestHeapRemove(vector<int>{1, 2, 3, 4, 5}, -3);
    TestHeapRemove(vector<int>{1, 2, 3, 4, 5}, 10);
    TestHeapRemove(vector<int>{3, 8, 2, 1, 4}, 1);
    TestHeapRemove(vector<int>{5, 5, 5, 5, 5}, 2);

    srand(07012014);
    const size_t smallTestCount = 1000;
    for (size_t testNum = 1; testNum <= smallTestCount; ++testNum) {
        cout << "Test " << testNum << endl;
        StressTestRemove(10, 10, 1);
    }

    const size_t bigTestCount = 1000;
    for (size_t testNum = smallTestCount + 1; testNum <= bigTestCount; ++testNum) {
        cout << "Test " << testNum << endl;
        StressTestRemove(100, 1000, 10);
    }
}

void TestHeapAll() {
    cout << "Testing single parameter constructor of the Heap" << endl;
    TestHeapConstructorAll();

    cout << "Testing insert of the Heap" << endl;
    TestHeapInsertAll();

    cout << "Testing remove of the Heap" << endl;
    TestHeapRemoveAll();
}

void TestMemoryManage(int size, const vector<int>& rawRequests, const vector<int>& answers) {
    MemoryManager manager(size);
    vector<int> results;
    results.reserve(rawRequests.size());

    for (int i = 0; i < rawRequests.size(); ++i) {
        Request* request;
        if (rawRequests[i] >= 0) {
            request = new AllocationRequest(rawRequests[i]);
        } else {
            request = new RevocationRequest(-rawRequests[i]);
        }
        int result = processRequest(manager, *request);
        if (request->type() == ALLOCATION) {
            results.push_back(result);
        }
    }

    CheckResult(rawRequests, results, answers, "processRequest");
}

void TestMemoryManageAll() {
    TestMemoryManage(6, vector<int>{2, 2, 2, -1, -2, -3}, vector<int>{1, 3, 5});
    TestMemoryManage(1, vector<int>{5, 5, 5, 5, 5}, vector<int>{-1, -1, -1, -1, -1});
    TestMemoryManage(6, vector<int>{6, -1, 6, -3, 6}, vector<int>{1, 1, 1});
    TestMemoryManage(6, vector<int>{-3, -2, -1, 4, 2, 1}, vector<int>{1, 5, -1});
    TestMemoryManage(6, vector<int>{2, 3, -1, 3, 3, -5, 2, 2}, vector<int>{1, 3, -1, -1, 1, -1});
}

void TestAll() {
    TestHeapAll();
    TestMemoryManageAll();
}
