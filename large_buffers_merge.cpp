#include "quicksort.cpp"

#include <vector>
#include <fstream>

using namespace std;

#define		N	1000
#define		b	50
#define		n	4

#define		FILENAME	"test_data.txt"

// Liczniki operacji blokowych (globalne zmienne symulujące statystyki)
int readCount = 0;
int writeCount = 0;

struct Record {
    double sides[5];
};

double perimeter (const Record &p) {
	return p.sides[0] + p.sides[1] + p.sides[2] + p.sides[3] + p.sides[4];
}

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
        readCount++;
        return true;
    }
    return false;  // koniec pliku
}

// Symulacja zapisu jednego rekordu do pliku binarnego
void write_record(ofstream &f, const Record &p) {
    f.write((char*)&p, sizeof(Record));
    writeCount++;
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
    while (readRecord(in, record)) {               // 🔸 używamy funkcji warstwy I/O
        buffer.push_back(record);

        if ((int)buffer.size() == runSize) {
            sort(buffer.begin(), buffer.end(), compareByPerimeter);

            string runName = "run" + to_string(runIndex) + ".dat";
            ofstream out(runName.c_str(), ios::binary);

            for (size_t i = 0; i < buffer.size(); i++) {
                writeRecord(out, buffer[i]);       // 🔸 używamy funkcji warstwy I/O
            }

            out.close();
            buffer.clear();
            runIndex++;
        }
    }

    // Zapisz pozostałości (ostatnią, niepełną serię)
    if (!buffer.empty()) {
        sort(buffer.begin(), buffer.end(), compareByPerimeter);
        string runName = "run" + to_string(runIndex) + ".dat";
        ofstream out(runName.c_str(), ios::binary);
        for (size_t i = 0; i < buffer.size(); i++) {
            writeRecord(out, buffer[i]);
        }
        out.close();
    }

    in.close();
}

int main()
{

}