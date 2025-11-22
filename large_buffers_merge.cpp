#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <queue>

using namespace std;

//#define		NUMBER_OF_RECORDS	1000	//N
#define		BLOCKING_FACTOR	2		//b
#define		NUMBER_OF_BUFFERS	3		//n

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
    int run_id; //index of run in which the record was

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
void create_runs(vector<string> &runs, const string &inputFile, int runSize) {
    ifstream in(inputFile.c_str(), ios::binary);
    if (!in.is_open()) {
        cerr << "Błąd: nie można otworzyć pliku " << inputFile << endl;
        return;
    }

    int runIndex = 0;
    int counter = 0;
    Record buffer[NUMBER_OF_BUFFERS * BLOCKING_FACTOR];

    Record record;
    while (read_record(in, record)) {               // 🔸 używamy funkcji warstwy I/O
        //buffer.push_back(record);
        buffer[counter] = record;
        counter++;

        if (counter == runSize) {
            qsort(&buffer[0], runSize, sizeof(Record), compare_records);

            string run_name = "run" + to_string(runIndex) + ".dat";
            ofstream out(run_name.c_str(), ios::binary);

            for (size_t i = 0; i < runSize; i++) {
                write_record(out, buffer[i]);       // 🔸 używamy funkcji warstwy I/O
            }

            out.close();
            print_dat_file(run_name);
			counter = 0;
            runs.push_back(run_name);
            runIndex++;
        }
    }

    // Zapisz pozostałości (ostatnią, niepełną serię)
    if (counter > 0) {
        qsort(&buffer[0], counter, sizeof(Record), compare_records);
        string run_name = "run" + to_string(runIndex) + ".dat";
        ofstream out(run_name.c_str(), ios::binary);
        for (size_t i = 0; i < counter; i++) {
            write_record(out, buffer[i]);
        }
        out.close();
		print_dat_file(run_name);
        runs.push_back(run_name);
    }

    in.close();
}

void reset_end_of_input(bool end_of_input[NUMBER_OF_BUFFERS-1])
{
    for (int i = 0; i<NUMBER_OF_BUFFERS-1; i++)
    {
        end_of_input[i] = false;
    }
}

bool all_inputs_empty(bool end_of_input[NUMBER_OF_BUFFERS-1])
{
    for (int i = 0; i<NUMBER_OF_BUFFERS-1; i++)
    {
        if (!end_of_input[i])
        {
            return false;
        }
    }
    return true;
}

/*//for debugging
void print_buffer(Record buffers[BLOCKING_FACTOR])
{
    for(int j = 0; j<BLOCKING_FACTOR;j++)
    {
        for(int i = 0; i<5; i++)
        {
            cout<<buffers[j].sides[i]<<" ";
        }
        cout<<endl;
    }
}*/

void merge_runs(const vector<string> &input_runs, const string &final_output_run)
{
    int input_runs_number = input_runs.size();

    if (input_runs_number == 1)
    {
        rename(input_runs[0].c_str(), final_output_run.c_str());     //the only output run left is the final one
        return;
    }

    priority_queue<HeapNode, vector<HeapNode>, greater<HeapNode>> heap;     //initializing min-heap

    vector<string> merged_runs;     //vector of names of merged runs

    int merged_runs_number = 0;     //to give names to merged files
    int i = 0;

    while (i<input_runs_number)           //repeat until reaching the end of the file (process all input runs)
    {
        //Buffers
        Record buffers[NUMBER_OF_BUFFERS - 1][BLOCKING_FACTOR];
        Record output_buffer[BLOCKING_FACTOR];
        
        //Subsidiary arrays
        ifstream inputs [NUMBER_OF_BUFFERS - 1];    //run FILES to read in this cycle
        bool end_of_input [NUMBER_OF_BUFFERS - 1];  //flag if we reached end of the input file

        int buffers_to_process = NUMBER_OF_BUFFERS-1;       //n-1 by default, changed when we reach the final run

        reset_end_of_input(end_of_input);   //mark all n-1 input runs as not empty

        //Opening n-1 files and loading first b records to buffers
        for (int j = 0; j<buffers_to_process; j++)
        {
            //opening files
            inputs[j].open(input_runs[i], ios::binary);
            if (!inputs[j].is_open()) {
                cerr << "Error: Couldn't open file " << input_runs[i] << endl;
                return;
            }

            //filling with first b records 
            int record_counter = 0;
            while(record_counter < BLOCKING_FACTOR)
            {
                Record record;
                if (read_record(inputs[j],record))
                {
                    buffers[j][record_counter] = record;        //filling buffer
                    record_counter++;
                }
                else
                {
                    for (int k = record_counter; k < BLOCKING_FACTOR-1; k++)        //if run has less than b records - the last file
                    {
                        buffers[j][k] = {-1, -1, -1, -1, -1};       //filling with "empty" records
                    }
                    buffers_to_process = j;
                    for(int k = j+1; k<NUMBER_OF_BUFFERS-1; k++)
                    {
                        end_of_input[k] = true;         //getting rid of unnecessary buffers <- backup - may be not needed
                    }
                    //i++;    //may not be needed, but won't harm
                    break;
                }
            }
            i++;
            if(i == input_runs_number)
            {
                buffers_to_process = j+1;
                break;
            }
        }

        //We have n-1 buffers filled with first b records

        //Record indexes - for counting records
        int output_record_id = 0;
        int *input_record_id = new int [buffers_to_process];
        for (int j = 0; j<buffers_to_process; j++)
        {
            input_record_id[j] = 0;
        }

        //Pushing first record from each file on the heap
        for(int j = 0; j<buffers_to_process; j++)
        {
            heap.push({buffers[j][0], j});
            if(!end_of_input[j])
            {
                Record record;
                if(read_record(inputs[j], record))
                {
                    buffers[j][0] = record;
                }
                else
                {
                    buffers[j][0] = {-1, -1, -1, -1, -1};
                    end_of_input[j] = true;
                }
            }
            input_record_id[j]++;
        }

        //opening the output file
        string merged_run_name = "merged" + to_string(merged_runs_number) + ".dat";
        ofstream out(merged_run_name, ios::binary);
        if (!out.is_open())
        {
            cerr << "Error: couldn't open file " << merged_run_name << endl;
            return;
        }

        while(!heap.empty())
        {
            //getting next record from the heap
            HeapNode node = heap.top();
            heap.pop();

            //saving the record into the output buffer
            if(perimeter(node.rec)>0)        //last buffer partly filled with records {-1,-1,-1,-1,-1}
            {
                output_buffer[output_record_id] = node.rec;
                output_record_id++;
            }

            //saving the page if the output buffer is filled
            if(output_record_id == BLOCKING_FACTOR)
            {
                output_record_id = 0;
                for(int j = 0; j<BLOCKING_FACTOR; j++)
                {
                    write_record(out, output_buffer[j]);
                }
            }

            //putting next record in the buffer(next from run file) and heap(next from the buffer) if possible
            int current_run_id = node.run_id;
            if(perimeter(buffers[current_run_id][input_record_id[current_run_id]])>0)
            {
                heap.push({buffers[current_run_id][input_record_id[current_run_id]], current_run_id});
            }
            if(end_of_input[current_run_id])
            {
                buffers[current_run_id][input_record_id[current_run_id]] = {-1, -1, -1, -1, -1};
            }
            else
            {
                Record next_record;
                if(read_record(inputs[current_run_id], next_record))
                {
                    buffers[current_run_id][input_record_id[current_run_id]] = next_record;
                }
                else
                {
                    end_of_input[current_run_id] = true;
                    buffers[current_run_id][input_record_id[current_run_id]] = {-1, -1, -1, -1, -1};//?
                }
            }
            input_record_id[current_run_id] = (input_record_id[current_run_id] + 1) % BLOCKING_FACTOR;
        }

        //saving the last, unfilled page
        for(int j = 0; j<output_record_id; j++)
        {
            write_record(out, output_buffer[j]);
        }

        merged_runs.push_back(merged_run_name);
        merged_runs_number++;
            

        //Cleanup
        for (int j = 0; j<buffers_to_process; j++)
        {
            inputs[j].close();
        }
        out.close();
        delete [] input_record_id;

        //Printing
        if(show_sorting_phases)
        {
            cout<<"Results of sorting:"<<endl;
            print_dat_file(merged_run_name);
        }
    }

    merge_runs(merged_runs, final_output_run);   //repeat until there is only 1 run left

}

int main()
{
	txt_to_dat(TXT_FILENAME, DAT_FILENAME); 
	print_dat_file(DAT_FILENAME);

    vector<string> runs;
	create_runs(runs, DAT_FILENAME, BLOCKING_FACTOR*NUMBER_OF_BUFFERS);

    // Scalamy wszystkie runy do jednego pliku
    string mergedFile = "merged_output.dat";
    merge_runs(runs, mergedFile);

    cout << "\nScalony plik:\n";
    print_dat_file(mergedFile);
	
	return 0;
}