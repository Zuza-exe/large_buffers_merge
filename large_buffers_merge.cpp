#include <iostream>
#include <vector>
#include <fstream>

using namespace std;

//#define		N	1000
//#define		b	50
//#define		n	4

#define		TXT_FILENAME	"test_data.txt"
#define		DAT_FILENAME	"test_data.dat"

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

int main()
{
	txt_to_dat(TXT_FILENAME, DAT_FILENAME); 
	print_dat_file(DAT_FILENAME);
	create_runs(DAT_FILENAME, 5);
	return 0;
}