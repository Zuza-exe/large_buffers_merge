#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <queue>

using namespace std;

#define		NUMBER_OF_RECORDS	1000	//N
#define		BLOCKING_FACTOR	50		//b
#define		NUMBER_OF_BUFFERS	4		//n

#define		TXT_FILENAME	"test_data.txt"
#define		DAT_FILENAME	"test_data.dat"

// Flags
bool random_records = true;		// true - random, false - manually
bool show_sorting_phases = true;


// Liczniki operacji blokowych (globalne zmienne symulujące statystyki)
int read_count = 0;
int write_count = 0;
int phase_count = 0;

struct Record {
    double sides[5];
};

double perimeter (const Record &p) {
	return p.sides[0] + p.sides[1] + p.sides[2] + p.sides[3] + p.sides[4];
}

struct HeapNode {
    Record rec;
    int idx; // indeks pliku/runu

    bool operator>(const HeapNode &other) const {
        return perimeter(rec) > perimeter(other.rec); // minimalny rekord na szczycie
    }
};

int compare_records(const void *a, const void *b) {
    const Record *pa = (const Record*)a;
    const Record *pb = (const Record*)b;

    double perA = perimeter(*pa);
    double perB = perimeter(*pb);

    if (perA < perB) return -1;
    if (perA > perB) return 1;
    return 0;
}

// Symulacja odczytu jednego rekordu z pliku binarnego
bool read_record(ifstream &f, Record &p) {
    if (f.read((char*)&p, sizeof(Record))) {
        read_count++;
        return true;
    }
    return false;  // koniec pliku
}

// Symulacja zapisu jednego rekordu do pliku binarnego
void write_record(ofstream &f, const Record &p) {
    f.write((char*)&p, sizeof(Record));
    write_count++;
}

void txt_to_dat(const string &txt, const string &dat) {
    ifstream in(txt);
    ofstream out(dat, ios::binary | ios::out | ios::trunc);  // TRUNCATE + OUT

    if (!in.is_open()) {
        cerr << "Nie można otworzyć pliku tekstowego!" << endl;
        return;
    }
    if (!out.is_open()) {
        cerr << "Nie można otworzyć pliku binarnego do zapisu!" << endl;
        return;
    }

    Record r;
    while (true) {
        for (int i = 0; i < 5; i++) {
            if (!(in >> r.sides[i])) {
                return; // koniec pliku TXT
            }
        }
        out.write((char*)&r, sizeof(Record));
    }
}

void print_dat_file(const string &filename) {
    ifstream in(filename, ios::binary);
    if (!in.is_open()) {
        cerr << "Nie mogę otworzyć pliku: " << filename << endl;
        return;
    }

    Record p;
    int index = 0;

    cout << "Zawartosc pliku " << filename << ":\n\n";

    while (in.read((char*)&p, sizeof(Record))) {
        cout << "Rekord " << index++ << ":  ";
        for (int i = 0; i < 5; i++)
            cout << p.sides[i] << " ";
        cout << " | obwód = " << perimeter(p) << endl;
    }
	cout<<endl;

    in.close();
}

// Tworzenie początkowych serii
void create_runs(const string &inputFile, int runSize) {
    ifstream in(inputFile.c_str(), ios::binary);
    if (!in.is_open()) {
        cerr << "Błąd: nie można otworzyć pliku " << inputFile << endl;
        return;
    }

    int runIndex = 0;
    vector<Record> buffer;
    buffer.reserve(runSize);

    Record record;
    while (read_record(in, record)) {               // 🔸 używamy funkcji warstwy I/O
        buffer.push_back(record);

        if ((int)buffer.size() == runSize) {
            qsort(&buffer[0], buffer.size(), sizeof(Record), compare_records);

            string run_name = "run" + to_string(runIndex) + ".dat";
            ofstream out(run_name.c_str(), ios::binary);

            for (size_t i = 0; i < buffer.size(); i++) {
                write_record(out, buffer[i]);       // 🔸 używamy funkcji warstwy I/O
            }

            out.close();
            print_dat_file(run_name);
			buffer.clear();
            runIndex++;
        }
    }

    // Zapisz pozostałości (ostatnią, niepełną serię)
    if (!buffer.empty()) {
        qsort(&buffer[0], buffer.size(), sizeof(Record), compare_records);
        string run_name = "run" + to_string(runIndex) + ".dat";
        ofstream out(run_name.c_str(), ios::binary);
        for (size_t i = 0; i < buffer.size(); i++) {
            write_record(out, buffer[i]);
        }
		
        out.close();
		print_dat_file(run_name);
    }

    in.close();
}

void merge_runs(const vector<string> &inputRuns, const string &outputRun) {
    int k = inputRuns.size();

    // Otwieramy wszystkie pliki wejściowe
    vector<ifstream> inputs(k);
    for (int i = 0; i < k; i++) {
        inputs[i].open(inputRuns[i], ios::binary);
        if (!inputs[i].is_open()) {
            cerr << "Błąd: nie mogę otworzyć " << inputRuns[i] << endl;
            return;
        }
    }

    ofstream out(outputRun, ios::binary);
    if (!out.is_open()) {
        cerr << "Błąd: nie mogę otworzyć wyjściowego " << outputRun << endl;
        return;
    }

    // Min-heap
    priority_queue<HeapNode, vector<HeapNode>, greater<HeapNode>> heap;

    // Inicjalizacja heap: wczytujemy pierwszy rekord z każdego pliku
    for (int i = 0; i < k; i++) {
        Record rec;
        if (read_record(inputs[i], rec)) {
            heap.push({rec, i});
        }
    }

    // Scalanie
    while (!heap.empty()) {
        HeapNode node = heap.top();
        heap.pop();

        write_record(out, node.rec);

        Record nextRec;
        if (read_record(inputs[node.idx], nextRec)) {
            heap.push({nextRec, node.idx});
        }
    }

    // Sprzątanie
    for (auto &f : inputs) f.close();
    out.close();
}


int main()
{
	txt_to_dat(TXT_FILENAME, DAT_FILENAME); 
	print_dat_file(DAT_FILENAME);
	create_runs(DAT_FILENAME, 5);
	
	// Przygotowujemy listę wszystkich utworzonych runów
    vector<string> runs;
    for (int i = 0; ; i++) {
        string run_name = "run" + to_string(i) + ".dat";
        ifstream f(run_name, ios::binary);
        if (!f.is_open()) break; // koniec listy runów
        runs.push_back(run_name);
    }

    // Scalamy wszystkie runy do jednego pliku
    string mergedFile = "merged_output.dat";
    merge_runs(runs, mergedFile);

    cout << "\nScalony plik:\n";
    print_dat_file(mergedFile);
	
	return 0;
}