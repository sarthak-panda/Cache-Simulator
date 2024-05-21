#include <iostream>
#include <vector>
#include <utility>
#include <fstream>
#include <sstream>
#include <list>
#include <unordered_map>
#include <queue>
#include <limits>
using namespace std;
class Cache
{
private:
    int no_of_sets;
    int no_of_blocks;
    int capacity_of_blocks;
    vector<vector<bool>> valid;
    vector<vector<bool>> dirtybit;
    vector<vector<int>> tag;
    vector<vector<int>> lrutag;
    vector<int> fifotag;       // queue containg line nos of a particular set
    int cache_access_time = 1; // it is preassumed that processor will ask for 1 byte data so independent of data size
    int main_memory_access_time_4_bytes = 100;
    bool write_allocate;
    bool write_through_or_write_back;
    bool lru_or_fifo;
    vector<pair<char, unsigned int>> trace_file;
    int total_cycle = 0;
    int load_hit = 0;
    int load_miss = 0;
    int store_hit = 0;
    int store_miss = 0;
    int total_load = 0;
    int total_store = 0;

    int get_set_index(unsigned int address)
    {
        return (address / this->capacity_of_blocks) % (this->no_of_sets);
    }
    int get_tag(unsigned int address)
    {
        return (address / (this->no_of_sets * this->capacity_of_blocks));
    }
    int check_hit_get_way_index(unsigned int address)
    {
        int set_index = this->get_set_index(address);
        int address_tag = this->get_tag(address);
        for (int i = 0; i < this->no_of_blocks; i++)
        {
            if (this->valid[set_index][i] == 1 && this->tag[set_index][i] == address_tag)
            {
                return i; // hit
            }
        }
        return -1; // miss
    }
    int check_compulsory_miss_get_free_line(unsigned int address)
    {
        int set_index = this->get_set_index(address);
        for (int i = 0; i < this->no_of_blocks; i++)
        {
            if (this->valid[set_index][i] == 0)
            {
                return i; // empty location was found
            }
        }
        return -1;
    }
    void update_lru_tags()
    {
        for (int i = 0; i < this->no_of_sets; i++)
        {
            for (int j = 0; j < this->no_of_blocks; j++)
            {
                if (this->lrutag[i][j] != -1)
                {
                    this->lrutag[i][j]++;
                }
            }
        }
    }
    int get_lru(int set_id)
    {
        int res = -1;
        int max_lrutag = std::numeric_limits<int>::min();
        for (int i = 0; i < this->no_of_blocks; i++)
        {
            if (this->lrutag[set_id][i] > max_lrutag)
            {
                max_lrutag = this->lrutag[set_id][i];
                res = i;
            }
        }
        return res;
    }

public:
    Cache(int s, int b, int bc, bool wa, bool wt_or_wb, bool l_or_fifo)
    {
        this->no_of_sets = s;
        this->no_of_blocks = b;
        this->capacity_of_blocks = bc;
        this->write_allocate = wa;
        this->write_through_or_write_back = wt_or_wb;
        this->lru_or_fifo = l_or_fifo;
        vector<bool> u(b, false);
        vector<vector<bool>> v(s, u);
        this->valid = v;
        this->dirtybit = v;
        vector<int> l(b, -1);
        vector<vector<int>> p(s, l);
        this->tag = p;
        this->lrutag = p;
        vector<int> q(s, 0);
        this->fifotag = q;
    }
    void set_trace_file(vector<pair<char, unsigned int>> t_f)
    {
        this->trace_file = t_f;
    }
    void simulate_cache()
    {
        for (auto p : this->trace_file)
        {
            if (this->lru_or_fifo)
            {
                this->update_lru_tags();
            }
            int curr_set_index = this->get_set_index(p.second);
            int curr_line_if_hit = this->check_hit_get_way_index(p.second);
            if (curr_line_if_hit != -1)
            { // hit
                if (this->lru_or_fifo)
                {
                    this->lrutag[curr_set_index][curr_line_if_hit] = 0;
                }
                if (p.first == 'l')
                {
                    this->load_hit++;
                    this->total_load++;
                    // processor take back data
                    this->total_cycle += this->cache_access_time;
                    // done
                }
                else
                { //'s'
                    this->store_hit++;
                    this->total_store++;
                    // make change in cache
                    this->total_cycle += this->cache_access_time;
                    if (this->write_through_or_write_back)
                    { // write through
                        // update the main memory
                        this->total_cycle += (this->main_memory_access_time_4_bytes) * ((this->capacity_of_blocks + 4 - 1) / 4);
                        // done
                    }
                    else
                    {                                                            // write back
                        this->dirtybit[curr_set_index][curr_line_if_hit] = true; 
                        // done
                    }
                }
            }
            else
            { // miss
                int free_line = this->check_compulsory_miss_get_free_line(p.second);
                if (p.first == 'l')
                {
                    this->load_miss++;
                    this->total_load++;
                    if (free_line != -1)
                    { // empty space found
                        if (this->lru_or_fifo)
                        {
                            this->lrutag[curr_set_index][free_line] = 0;
                        }
                        this->valid[curr_set_index][free_line] = true;
                        // bring the data frm main memory to cache
                        this->tag[curr_set_index][free_line] = get_tag(p.second);
                        this->total_cycle += (this->main_memory_access_time_4_bytes) * ((this->capacity_of_blocks + 4 - 1) / 4); // read from main memory
                        this->total_cycle += this->cache_access_time;                                                            // write to cache memory
                        // processor accesses the loaded data(here we neglect the modelling the exact data fetch using offest that we need during making a cache)
                        this->total_cycle += this->cache_access_time;
                        // done
                    }
                    else
                    { // conflict or capacity miss
                        int curr_line;
                        if (this->lru_or_fifo)
                        { // lru
                            curr_line = this->get_lru(curr_set_index);
                            this->lrutag[curr_set_index][curr_line] = 0;
                        }
                        else
                        { // fifo
                            curr_line = this->fifotag[curr_set_index];
                            this->fifotag[curr_set_index] = (this->fifotag[curr_set_index] + 1) % (this->no_of_blocks);
                        }
                        if (!(this->write_through_or_write_back) && this->dirtybit[curr_set_index][curr_line])
                        {
                            // update the main memory and send back the data
                            this->total_cycle += this->cache_access_time;                                                            // read from cache
                            this->total_cycle += (this->main_memory_access_time_4_bytes) * ((this->capacity_of_blocks + 4 - 1) / 4); // write to main memory
                            this->dirtybit[curr_set_index][curr_line] = false;                                                       // update dirty bit to zero for new data upcoming
                        }
                        // bring data from main memory to cache
                        this->tag[curr_set_index][curr_line] = get_tag(p.second);
                        this->total_cycle += (this->main_memory_access_time_4_bytes) * ((this->capacity_of_blocks + 4 - 1) / 4); // read from main memory
                        this->total_cycle += this->cache_access_time;                                                            // write to cache
                        // valid bit remains to be 1
                        // processor loads this fetched data from cache
                        this->total_cycle += this->cache_access_time;
                        // done
                    }
                }
                else
                { //'s'
                    this->store_miss++;
                    this->total_store++;
                    if (free_line != -1)
                    { // compulsory miss
                        if (this->write_allocate)
                        {
                            if (this->lru_or_fifo)
                            {
                                this->lrutag[curr_set_index][free_line] = 0;
                            }
                            this->valid[curr_set_index][free_line] = true;
                            // bring the data from  main memory
                            this->tag[curr_set_index][free_line] = get_tag(p.second);
                            this->total_cycle += (this->main_memory_access_time_4_bytes) * ((this->capacity_of_blocks + 4 - 1) / 4);
                            this->total_cycle += this->cache_access_time;
                            this->total_cycle += this->cache_access_time;
                            if (this->write_through_or_write_back)
                            { // write through
                                // update the main memory
                                this->total_cycle += (this->main_memory_access_time_4_bytes) * ((this->capacity_of_blocks + 4 - 1) / 4);
                                // done
                            }
                            else
                            { // write back
                                this->dirtybit[curr_set_index][free_line] = true;
                                // done
                            }
                        }
                        else
                        { // no write allocate
                          // directly write to main memory(we will beassuming same 100 cycle thing) cache is left untouched
                          // update the main memory
                            this->total_cycle += (this->main_memory_access_time_4_bytes) * ((this->capacity_of_blocks + 4 - 1) / 4);
                            // done
                        }
                    }
                    else
                    { // conflict or capacity miss
                        // valid bit continues to be one
                        if (this->write_allocate)
                        {
                            int curr_line;
                            if (this->lru_or_fifo)
                            { // lru
                                curr_line = this->get_lru(curr_set_index);
                                this->lrutag[curr_set_index][curr_line] = 0;
                            }
                            else
                            { // fifo
                                curr_line = this->fifotag[curr_set_index];
                                this->fifotag[curr_set_index] = (this->fifotag[curr_set_index] + 1) % (this->no_of_blocks);
                            }
                            if (!(this->write_through_or_write_back) && this->dirtybit[curr_set_index][curr_line])
                            {
                                // update the main memory and send back the data
                                this->total_cycle += this->cache_access_time;
                                this->total_cycle += (this->main_memory_access_time_4_bytes) * ((this->capacity_of_blocks + 4 - 1) / 4);
                                this->dirtybit[curr_set_index][curr_line] = false; // update dirty bit to zero for new data upcoming
                            }
                            // bring the data from  main memory
                            this->tag[curr_set_index][curr_line] = get_tag(p.second);
                            this->total_cycle += (this->main_memory_access_time_4_bytes) * ((this->capacity_of_blocks + 4 - 1) / 4);
                            this->total_cycle += this->cache_access_time;
                            this->total_cycle += this->cache_access_time;
                            if (this->write_through_or_write_back)
                            { // write through
                                // update the main memory
                                this->total_cycle += (this->main_memory_access_time_4_bytes) * ((this->capacity_of_blocks + 4 - 1) / 4);
                                // done
                            }
                            else
                            { // write back
                                this->dirtybit[curr_set_index][curr_line] = true;
                                // done
                            }
                        }
                        else
                        { // no write allocate
                            // directly write to main memory(we will beassuming same 100 cycle thing) cache is left untouched
                            // update the main memory
                            this->total_cycle += (this->main_memory_access_time_4_bytes) * ((this->capacity_of_blocks + 4 - 1) / 4);
                            // done
                        }
                    }
                }
            }
        }
    }
    int get_total_cycles()
    {
        return this->total_cycle;
    }
    int get_total_load_hit()
    {
        return this->load_hit;
    }
    int get_total_load_miss()
    {
        return this->load_miss;
    }
    int get_total_store_hit()
    {
        return this->store_hit;
    }
    int get_total_store_miss()
    {
        return this->store_miss;
    }
    int get_total_load()
    {
        return this->total_load;
    }
    int get_total_store()
    {
        return this->total_store;
    }
};

int hexStringToInt(const std::string &hexStr)
{
    std::stringstream ss;
    ss << std::hex << hexStr;
    unsigned int result;
    ss >> result;
    return result;
}

void store_trace_file(string filename, vector<pair<char, unsigned int>> &result)
{
    std::ifstream infile(filename);
    if (!infile.is_open())
    {
        std::cerr << "Error opening file." << std::endl;
    }
    std::string line;
    while (std::getline(infile, line))
    {
        std::istringstream iss(line);
        char action;
        std::string value;
        if (iss >> action >> value)
        {
            if (value.size() >= 2 && value.substr(0, 2) == "0x")
            {
                value = value.substr(2);
            }
            unsigned int vl = hexStringToInt(value);
            result.push_back({action, vl});
        }
    }
    infile.close();
}
int main(int argc, char *argv[])
{
    if (argc != 7)
    {
        cout << "Incorrect number of parameters" << endl;
    }
    bool wa = false;
    if (std::string(argv[4]) == "write-allocate")
    {
        wa = true;
    }
    bool wt_or_wb = true;
    if (std::string(argv[5]) == "write-back")
    {
        wt_or_wb = false;
    }
    bool lru_or_fifo = false;
    if (std::string(argv[6]) == "lru")
    {
        lru_or_fifo = true;
    }
    Cache c(stoi(argv[1]), stoi(argv[2]), stoi(argv[3]), wa, wt_or_wb, lru_or_fifo);
    vector<pair<char, unsigned int>> v;
    // store_trace_file(argv[7], v);
    // Read from stdin
    string line;
    while (getline(cin, line)) {
        std::istringstream iss(line);
        char action;
        std::string value;
        if (iss >> action >> value)
        {
            if (value.size() >= 2 && value.substr(0, 2) == "0x")
            {
                value = value.substr(2);
            }
            unsigned int vl = hexStringToInt(value);
            v.push_back({action, vl});
        }
    }
    c.set_trace_file(v);
    c.simulate_cache();
    cout << "Total loads: " << c.get_total_load() << endl;
    cout << "Total stores: " << c.get_total_store() << endl;
    cout << "Load hits: " << c.get_total_load_hit() << endl;
    cout << "Load misses: " << c.get_total_load_miss() << endl;
    cout << "Store hits: " << c.get_total_store_hit() << endl;
    cout << "Store misses: " << c.get_total_store_miss() << endl;
    cout << "Total cycles: " << c.get_total_cycles() << endl;

    return 0;
}