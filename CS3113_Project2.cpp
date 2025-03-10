#include <iostream>
#include <queue>
#include <vector>
#include <map>
#include <climits>  // for INT_MAX
using namespace std;

// Process States.
enum ProcessState { NEW = 1, READY, RUNNING, IOWAITING, TERMINATED };

// Forward declaration of the PCB struct.
struct PCB;

// PCB definition.
struct PCB {
    int processID;
    int state;            // NEW, READY, RUNNING, IOWAITING, or TERMINATED.
    int programCounter;   // Stored in mainMemory at mainMemoryBase + 2.
    int instructionBase;
    int dataBase;
    int memoryLimit;
    int cpuCyclesUsed;    // Aggregated cycles used over all slices.
    int registerValue;
    int maxMemoryNeeded;
    int mainMemoryBase;
    vector<vector<int> > operations;
    
    // Simulation fields:
    int CPUAllocated;            // Time slice allowed.
    int startRunningTime;        // When the process first entered Running.
    int terminationTime;         // Recorded when the process terminates.
    int currentInstructionIndex; // Next instruction index (0-based).
    int remainingInstructions;   // Number of instructions left.
    int ioReleaseTime;           // Global clock time when I/O wait ends.
    bool pendingPrint;           // True if a print is pending.
    
    PCB() : processID(0), state(NEW), programCounter(0), instructionBase(0), dataBase(0),
            memoryLimit(0), cpuCyclesUsed(0), registerValue(0), maxMemoryNeeded(0),
            mainMemoryBase(0), CPUAllocated(0), startRunningTime(-1), terminationTime(0),
            currentInstructionIndex(0), remainingInstructions(0), ioReleaseTime(0),
            pendingPrint(false) {}
};

//
// Checks the IOWaitingQueue. If a process's ioReleaseTime has passed, prints its I/O message and moves it to the ReadyQueue.
//
void checkIOQueue(int globalClock, queue<PCB*>& ioWaitingQueue, queue<PCB*>& readyQueue) {
    queue<PCB*> tempQueue;
    while (!ioWaitingQueue.empty()) {
        PCB* ioProc = ioWaitingQueue.front();
        ioWaitingQueue.pop();


        if (globalClock >= ioProc->ioReleaseTime) {
            cout << "print" << endl;
            cout << "Process " << ioProc->processID 
                 << " completed I/O and is moved to the ReadyQueue." << endl;
            ioProc->state = READY;
            ioProc->remainingInstructions--;
            readyQueue.push(ioProc);
        } else {
            tempQueue.push(ioProc);
        }
    }
    while (!tempQueue.empty()) {
        ioWaitingQueue.push(tempQueue.front());
        tempQueue.pop();
    }
}

//
// Loads the PCB header, instructions, and data into main memory.
//
void loadJobsToMemory(queue<PCB>& newJobQueue, queue<int>& readyMemoryQueue, vector<int>& mainMemory, int maxMemory) {
    for (int i = 0; i < maxMemory; i++)
        mainMemory.push_back(-1);
    
    while (!newJobQueue.empty()) {
        PCB workingJob = newJobQueue.front();
        newJobQueue.pop();
        int memoryIndex = workingJob.mainMemoryBase;
        // Store PCB header fields.
        mainMemory[memoryIndex]     = workingJob.processID;
        mainMemory[memoryIndex + 1] = workingJob.state;
        mainMemory[memoryIndex + 2] = workingJob.programCounter;
        mainMemory[memoryIndex + 3] = workingJob.instructionBase;
        mainMemory[memoryIndex + 4] = workingJob.dataBase;
        mainMemory[memoryIndex + 5] = workingJob.memoryLimit;
        mainMemory[memoryIndex + 6] = workingJob.cpuCyclesUsed;
        mainMemory[memoryIndex + 7] = workingJob.registerValue;
        mainMemory[memoryIndex + 8] = workingJob.maxMemoryNeeded;
        mainMemory[memoryIndex + 9] = workingJob.mainMemoryBase;
        memoryIndex += 10;
        // Load instructions.
        memoryIndex = workingJob.instructionBase;
        for (size_t i = 0; i < workingJob.operations.size(); i++) {
            mainMemory[memoryIndex] = workingJob.operations[i][0];
            memoryIndex++;
        }
        // Load data.
        memoryIndex = workingJob.dataBase;
        for (size_t i = 0; i < workingJob.operations.size(); i++) {
            for (size_t j = 1; j < workingJob.operations[i].size(); j++) {
                mainMemory[memoryIndex] = workingJob.operations[i][j];
                memoryIndex++;
            }
        }
    }
}

//
// Main simulation.
//
int main() {
    int maxMemory, CPUAllocated, contextSwitchTime, numProcesses;
    cin >> maxMemory >> CPUAllocated >> contextSwitchTime >> numProcesses;
    
    queue<PCB> newJobQueue;
    queue<int> readyMemoryQueue;
    vector<int> mainMemory;
    
    vector<PCB*> processList;
    queue<PCB*> readyQueue;
    queue<PCB*> ioWaitingQueue;
    map<int, int> terminationTimes; // Declare the terminationTimes map
    
    int processID, instructionCount;
    int inputType, incomingInput;
    int totalMem = 0;
    
    // Read each process.
    for (int i = 0; i < numProcesses; i++) {
        PCB proc;
        cin >> proc.processID >> proc.memoryLimit >> instructionCount;
        
        proc.remainingInstructions = instructionCount;
        proc.currentInstructionIndex = 0;
        proc.CPUAllocated = CPUAllocated;
        proc.startRunningTime = -1;
        proc.ioReleaseTime = 0;
        proc.pendingPrint = false;
        proc.mainMemoryBase = totalMem;
        proc.maxMemoryNeeded = proc.memoryLimit;
        proc.instructionBase = proc.mainMemoryBase + 10;
        proc.dataBase = proc.instructionBase + instructionCount;
        totalMem += proc.maxMemoryNeeded;
        
        // Read instructions.
        for (int j = 0; j < instructionCount; j++) {
            cin >> inputType;
            vector<int> currentOpcode;
            currentOpcode.push_back(inputType);
            switch(inputType) {
                case 1:
                    cin >> incomingInput;
                    currentOpcode.push_back(incomingInput);
                    cin >> incomingInput;
                    currentOpcode.push_back(incomingInput);
                    break;
                case 2:
                    cin >> incomingInput;
                    currentOpcode.push_back(incomingInput);
                    break;
                case 3:
                    cin >> incomingInput;
                    currentOpcode.push_back(incomingInput);
                    cin >> incomingInput;
                    currentOpcode.push_back(incomingInput);
                    break;
                case 4:
                    cin >> incomingInput;
                    currentOpcode.push_back(incomingInput);
                    break;
            }
            proc.operations.push_back(currentOpcode);
        }
        newJobQueue.push(proc);
        totalMem += 10; // Reserve space for PCB header.
        PCB* pProc = new PCB(proc);
        processList.push_back(pProc);
        readyQueue.push(pProc);
    }
    
    loadJobsToMemory(newJobQueue, readyMemoryQueue, mainMemory, maxMemory);
    
    // Dump main memory (for debugging purposes).
    for (int i = 0; i < maxMemory; i++) {
        cout << i << " : " << mainMemory[i] << endl;
    }
    
    int globalClock = 0;

    // Simulation loop.
    while (!readyQueue.empty() || !ioWaitingQueue.empty()) {
        // If readyQueue is empty but there are processes waiting on I/O, jump the clock.

        
        while (readyQueue.empty() && !ioWaitingQueue.empty()) {
            checkIOQueue(globalClock, ioWaitingQueue, readyQueue);
            if(readyQueue.empty())
                globalClock += contextSwitchTime;
        }
        
        // Context switch into the next process.
        PCB* currentProc = readyQueue.front();
        readyQueue.pop();
        globalClock += contextSwitchTime; // Context switch in.
        cout << "Process " << currentProc->processID << " has moved to Running." << endl;
        
        // Record the process's start time if this is its first scheduling.
        if (currentProc->startRunningTime == -1)
            currentProc->startRunningTime = globalClock;
        
        int sliceCycles = 0;
        bool ioOccurred = false;
        bool timeoutOccurred = false;
        
        // Execute instructions until the time slice expires or an I/O event occurs.
        while (currentProc->remainingInstructions > 0 && sliceCycles < currentProc->CPUAllocated) {
            vector<int>& instr = currentProc->operations[currentProc->currentInstructionIndex];
            int instrType = instr[0];
            
            if (instrType == 1) { // Compute.
                int cost = instr[2];
                cout << "compute" << endl;
                sliceCycles += cost;
                currentProc->cpuCyclesUsed += cost;

                globalClock += cost;
                mainMemory[currentProc->mainMemoryBase + 6] = currentProc->cpuCyclesUsed;
                
                currentProc->currentInstructionIndex++;
                currentProc->remainingInstructions--;
                mainMemory[currentProc->mainMemoryBase + 2] = currentProc->currentInstructionIndex;
                
                if (sliceCycles >= currentProc->CPUAllocated)
                    timeoutOccurred = true;
            }
            else if (instrType == 2) { // Print.
                int printCycles = instr[1];
                currentProc->cpuCyclesUsed += printCycles;
                mainMemory[currentProc->mainMemoryBase + 6] = currentProc->cpuCyclesUsed;
                currentProc->pendingPrint = true;
                currentProc->ioReleaseTime = globalClock + printCycles;
                cout << "Process " << currentProc->processID 
                     << " issued an IOInterrupt and moved to the IOWaitingQueue." << endl;
                
                currentProc->currentInstructionIndex++;
                mainMemory[currentProc->mainMemoryBase + 2] = currentProc->currentInstructionIndex;
                ioOccurred = true;
                break;
            }
            else if (instrType == 3) { // Store.
                cout << "stored" << endl;
                sliceCycles += 1;
                currentProc->cpuCyclesUsed += 1;
                globalClock += 1;
                mainMemory[currentProc->mainMemoryBase + 6] = currentProc->cpuCyclesUsed;
                int value = instr[1];
                int address = instr[2];
                if (address < currentProc->memoryLimit &&
                    (currentProc->mainMemoryBase + address) < mainMemory.size()) {
                    mainMemory[currentProc->mainMemoryBase + address] = value;
                    currentProc->registerValue = value;
                    mainMemory[currentProc->mainMemoryBase + 7] = value;
                } else {
                    cout << "store error!" << endl;
                }
                currentProc->currentInstructionIndex++;
                currentProc->remainingInstructions--;
                mainMemory[currentProc->mainMemoryBase + 2] = currentProc->currentInstructionIndex;
                
                if (sliceCycles >= currentProc->CPUAllocated)
                    timeoutOccurred = true;
            }
            else if (instrType == 4) { // Load.
                cout << "loaded" << endl;
                sliceCycles += 1;
                currentProc->cpuCyclesUsed += 1;
                globalClock += 1;
                mainMemory[currentProc->mainMemoryBase + 6] = currentProc->cpuCyclesUsed;
                int offset = instr[1];
                if (offset < currentProc->memoryLimit &&
                    (currentProc->mainMemoryBase + offset) < mainMemory.size()) {
                    currentProc->registerValue = mainMemory[currentProc->mainMemoryBase + offset];
                    mainMemory[currentProc->mainMemoryBase + 7] = currentProc->registerValue;
                } else {
                    cout << "load error!" << endl;
                    currentProc->registerValue = -1;
                    mainMemory[currentProc->mainMemoryBase + 7] = -1;
                }
                currentProc->currentInstructionIndex++;
                currentProc->remainingInstructions--;
                mainMemory[currentProc->mainMemoryBase + 2] = currentProc->currentInstructionIndex;
                
                if (sliceCycles >= currentProc->CPUAllocated)
                    timeoutOccurred = true;
            }
        } // End of time slice.

        if (currentProc->remainingInstructions > 0) {
            if (ioOccurred) {
                ioWaitingQueue.push(currentProc);
                checkIOQueue(globalClock, ioWaitingQueue, readyQueue);
            }
            else if (timeoutOccurred) {
                cout << "Process " << currentProc->processID 
                     << " has a TimeOUT interrupt and is moved to the ReadyQueue." << endl;
                readyQueue.push(currentProc);
                checkIOQueue(globalClock, ioWaitingQueue, readyQueue);
            }
            else {
                cout << "ERROR" << endl;
                readyQueue.push(currentProc);
            }
        }
        else {
            
            int finalPC = currentProc->mainMemoryBase + 9;
            mainMemory[currentProc->mainMemoryBase + 2] = finalPC;
            currentProc->terminationTime = globalClock;

            int totalCyclesConsumed = currentProc->terminationTime - currentProc->startRunningTime;
            cout << "Process ID: " << currentProc->processID << endl;
            cout << "State: TERMINATED" << endl;
            cout << "Program Counter: " << finalPC << endl;
            cout << "Instruction Base: " << currentProc->instructionBase << endl;
            cout << "Data Base: " << currentProc->dataBase << endl;
            cout << "Memory Limit: " << currentProc->memoryLimit << endl;
            cout << "CPU Cycles Used: " << currentProc->cpuCyclesUsed << endl;
            cout << "Register Value: " << currentProc->registerValue << endl;
            cout << "Max Memory Needed: " << currentProc->maxMemoryNeeded << endl;
            cout << "Main Memory Base: " << currentProc->mainMemoryBase << endl;
            
            cout << "Total CPU Cycles Consumed: " << totalCyclesConsumed << endl;
            cout << "Process " << currentProc->processID << " terminated. Entered running state at: " 
                 << currentProc->startRunningTime << ". Terminated at: " 
                 << currentProc->terminationTime << ". Total Execution Time: " 
                 << totalCyclesConsumed << "." << endl;
            
            // Record termination time in the map.
            terminationTimes[currentProc->processID] = currentProc->terminationTime;
            checkIOQueue(globalClock, ioWaitingQueue, readyQueue);
        }
    } // End simulation loop.
    
    // The overall Total CPU time used is defined as maxTerminationTime + final context switch time.
    cout << "Total CPU time used: " << (globalClock + contextSwitchTime) << "." << endl;
    
    // Clean up dynamically allocated memory.
    for (size_t i = 0; i < processList.size(); i++) {
        delete processList[i];
    }
    
    return 0;
}