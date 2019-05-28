#include <time.h>
#include <sys/time.h>

#include <algorithm>
#include <iostream>
#include <fstream>

#include "encoder_factory.hpp"
#include "btree_map.hpp"

//#define NOT_USE_ENCODE_PAIR 1

static const uint64_t kNumEmailRecords = 25000000;
static const uint64_t kNumWikiRecords = 14000000;
static const uint64_t kNumTxns = 10000000;
static const int kSamplePercent = 10;

static const std::string file_load_email = "workloads/load_email";
static const std::string file_load_wiki = "workloads/load_wiki";
static const std::string file_load_url = "workloads/load_url";

static const std::string file_txn_email = "workloads/txn_email_zipfian";
static const std::string file_txn_wiki = "workloads/txn_wiki_zipfian";
static const std::string file_txn_url = "workloads/txn_url_zipfian";

// for pretty print
static const char* kGreen ="\033[0;32m";
//static const char* kRed ="\033[0;31m";
static const char* kNoColor ="\033[0;0m";

//-------------------------------------------------------------
// Workload IDs
//-------------------------------------------------------------
static const int kEmail = 0;
static const int kWiki = 1;
static const int kUrl = 2;

//-------------------------------------------------------------
// Expt ID = 0
//-------------------------------------------------------------
static const std::string output_dir_btree_point = "results/btree/point/";
static const std::string file_lat_email_btree = output_dir_btree_point + "lat_email_btree.csv";
std::ofstream output_lat_email_btree;
static const std::string file_mem_email_btree = output_dir_btree_point + "mem_email_btree.csv";
std::ofstream output_mem_email_btree;

static const std::string file_lat_wiki_btree = output_dir_btree_point + "lat_wiki_btree.csv";
std::ofstream output_lat_wiki_btree;
static const std::string file_mem_wiki_btree = output_dir_btree_point + "mem_wiki_btree.csv";
std::ofstream output_mem_wiki_btree;

static const std::string file_lat_url_btree = output_dir_btree_point + "lat_url_btree.csv";
std::ofstream output_lat_url_btree;
static const std::string file_mem_url_btree = output_dir_btree_point + "mem_url_btree.csv";
std::ofstream output_mem_url_btree;

//-------------------------------------------------------------
// Expt ID = 1
//-------------------------------------------------------------
static const std::string output_dir_btree_range = "results/btree/range/";
static const std::string file_lat_email_btree_range = output_dir_btree_range + "lat_email_btree_range.csv";
std::ofstream output_lat_email_btree_range;
static const std::string file_mem_email_btree_range = output_dir_btree_range + "mem_email_btree_range.csv";
std::ofstream output_mem_email_btree_range;

static const std::string file_lat_wiki_btree_range = output_dir_btree_range + "lat_wiki_btree_range.csv";
std::ofstream output_lat_wiki_btree_range;
static const std::string file_mem_wiki_btree_range = output_dir_btree_range + "mem_wiki_btree_range.csv";
std::ofstream output_mem_wiki_btree_range;

static const std::string file_lat_url_btree_range = output_dir_btree_range + "lat_url_btree_range.csv";
std::ofstream output_lat_url_btree_range;
static const std::string file_mem_url_btree_range = output_dir_btree_range + "mem_url_btree_range.csv";
std::ofstream output_mem_url_btree_range;

double getNow() {
    struct timeval tv;
    gettimeofday(&tv, 0);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}

void loadKeysFromFile(const std::string& file_name, const uint64_t num_records,
		      std::vector<std::string> &keys) {
    std::ifstream infile(file_name);
    std::string key;
    uint64_t count = 0;
    while (count < num_records && infile.good()) {
	infile >> key;
	keys.push_back(key);
	count++;
    }
}

std::string getUpperBoundKey(const std::string& key) {
    std::string ret_str = key;
    ret_str[ret_str.size() - 1] = (char)'\255';
    return ret_str;
}

void loadWorkload(int wkld_id,
		  std::vector<std::string>& insert_keys,
		  std::vector<std::string>& insert_keys_sample,
		  std::vector<std::string>& txn_keys,
		  std::vector<std::string>& upper_bound_keys) {
    std::vector<std::string> load_keys;
    if (wkld_id == kEmail)
	loadKeysFromFile(file_load_email, kNumEmailRecords, load_keys);
    else if (wkld_id == kWiki)
	loadKeysFromFile(file_load_wiki, kNumWikiRecords, load_keys);
    else if (wkld_id == kUrl)
	loadKeysFromFile(file_load_url, kNumEmailRecords, load_keys);
    else
	return;

    std::sort(load_keys.begin(), load_keys.end());
    for (int i = 0; i < (int)load_keys.size() - 1; i++) {
	int key_len = load_keys[i].length();
	int next_key_len = load_keys[i + 1].length();
	if (key_len < next_key_len) {
	    std::string next_prefix = load_keys[i + 1].substr(0, key_len);
	    if (load_keys[i].compare(next_prefix) != 0)
		insert_keys.push_back(load_keys[i]);
	} else {
	    insert_keys.push_back(load_keys[i]);
	}
    }
    insert_keys.push_back(load_keys[load_keys.size() - 1]);

    load_keys.clear();
    std::random_shuffle(insert_keys.begin(), insert_keys.end());  
    
    for (int i = 0; i < (int)insert_keys.size(); i += (100 / kSamplePercent)) {
	insert_keys_sample.push_back(insert_keys[i]);
    }

    if (wkld_id == kEmail)
	loadKeysFromFile(file_txn_email, kNumTxns, txn_keys);
    else if (wkld_id == kWiki)
	loadKeysFromFile(file_txn_wiki, kNumTxns, txn_keys);
    else if (wkld_id == kUrl)
	loadKeysFromFile(file_txn_url, kNumTxns, txn_keys);
    
    for (int i = 0; i < (int)txn_keys.size(); i++) {
	upper_bound_keys.push_back(getUpperBoundKey(txn_keys[i]));
    }
    std::cout << "insert_keys size = " << insert_keys.size() << std::endl;
    std::cout << "insert_keys_sample size = " << insert_keys_sample.size() << std::endl;
    std::cout << "txn_keys size = " << txn_keys.size() << std::endl;
    std::cout << "upper_bound_keys size = " << upper_bound_keys.size() << std::endl;
}

void exec(const int expt_id, const int wkld_id, const bool is_point,
	  const bool is_compressed,
	  const int encoder_type, const int64_t dict_size_limit,
	  const std::vector<std::string>& insert_keys,
	  const std::vector<std::string>& insert_keys_sample,
	  const std::vector<std::string>& txn_keys,
	  const std::vector<std::string>& upper_bound_keys) {
    ope::Encoder* encoder = nullptr;
    uint8_t* buffer = new uint8_t[8192];
    uint8_t* buffer_r = new uint8_t[8192];
    std::vector<std::string> enc_insert_keys;

    int64_t total_key_size = 0;
    double start_time = getNow();
    if (is_compressed) {
	encoder = ope::EncoderFactory::createEncoder(encoder_type);
	encoder->build(insert_keys_sample, dict_size_limit);
	for (int i = 0; i < (int)insert_keys.size(); i++) {
	    int enc_len = encoder->encode(insert_keys[i], buffer);
	    int enc_len_round = (enc_len + 7) >> 3;
	    enc_insert_keys.push_back(std::string((const char*)buffer, enc_len_round));
	    total_key_size += enc_len_round;
	}
    } else {
	for (int i = 0; i < (int)insert_keys.size(); i++) {
	    enc_insert_keys.push_back(insert_keys[i]);
	    total_key_size += insert_keys[i].size();
	}
    }

    typedef tlx::btree_map<std::string, uint64_t, std::less<std::string> > btree_type;
    btree_type* bt = new btree_type();
    for (int i = 0; i < (int)enc_insert_keys.size(); i++) {
	bt->insert2(enc_insert_keys[i], (uint64_t)&(enc_insert_keys[i]));
    }

    double end_time = getNow();
    double build_time = end_time - start_time;
    std::cout << "Build time = " << build_time << std::endl;

    int64_t btree_size = 256 * bt->get_stats().nodes();
    double mem = (btree_size + total_key_size) / 1000000.0;
    std::cout << "Mem = " << mem << std::endl;

    // execute transactions =======================================
    uint64_t sum = 0;
    start_time = getNow();
    if (is_point) { // point query
	if (is_compressed) {
	    for (int i = 0; i < (int)txn_keys.size(); i++) {
		int enc_len = encoder->encode(txn_keys[i], buffer);
		int enc_len_round = (enc_len + 7) >> 3;
		std::string enc_key = std::string((const char*)buffer, enc_len_round);
		btree_type::const_iterator iter = bt->find(enc_key);
		sum += (iter->second);
	    }
	} else {
	    for (int i = 0; i < (int)txn_keys.size(); i++) {
		btree_type::const_iterator iter = bt->find(txn_keys[i]);
		sum += (iter->second);
	    }
	}
    } else { // range query
	if (is_compressed) {
	    for (int i = 0; i < (int)txn_keys.size(); i++) {
		int enc_len = 0, enc_len_r = 0;
#ifdef NOT_USE_ENCODE_PAIR
		enc_len = encoder->encode(txn_keys[i], buffer);
		enc_len_r = encoder->encode(upper_bound_keys[i], buffer_r);
#else
		encoder->encodePair(txn_keys[i], upper_bound_keys[i], buffer, buffer_r, enc_len, enc_len_r);
#endif
		int enc_len_round = (enc_len + 7) >> 3;
		int enc_len_r_round = (enc_len_r + 7) >> 3;
		std::string left_key = std::string((const char*)buffer, enc_len_round);
		std::string right_key = std::string((const char*)buffer_r, enc_len_r_round);

		btree_type::const_iterator iter = bt->lower_bound(left_key);
		while (iter.key().compare(right_key) < 0) {
		    ++iter;
		}
		sum += (iter->second);
	    }
	} else {
	    for (int i = 0; i < (int)txn_keys.size(); i++) {
		btree_type::const_iterator iter = bt->lower_bound(txn_keys[i]);
		while (iter.key().compare(upper_bound_keys[i]) < 0) {
		    ++iter;
		}
		sum += (iter->second);
	    }
	}
    }
    end_time = getNow();
    double exec_time = end_time - start_time;
    double tput = txn_keys.size() / exec_time / 1000000; // Mops/sec
    std::cout << kGreen << "Throughput = " << kNoColor << tput << "\n";
    double lat = (exec_time * 1000000) / txn_keys.size(); // us
    std::cout << kGreen << "Latency = " << kNoColor << lat << "\n";

    delete bt;

    if (expt_id == 0) {
	if (wkld_id == kEmail) {
	    output_lat_email_btree << lat << "\n";
	    output_mem_email_btree << mem << "\n";
	} else if (wkld_id == kWiki) {
	    output_lat_wiki_btree << lat << "\n";
	    output_mem_wiki_btree << mem << "\n";
	} else if (wkld_id == kUrl) {
	    output_lat_url_btree << lat << "\n";
	    output_mem_url_btree << mem << "\n";
	}
    } else if (expt_id == 1) {
	if (wkld_id == kEmail) {
	    output_lat_email_btree_range << lat << "\n";
	    output_mem_email_btree_range << mem << "\n";
	} else if (wkld_id == kWiki) {
	    output_lat_wiki_btree_range << lat << "\n";
	    output_mem_wiki_btree_range << mem << "\n";
	} else if (wkld_id == kUrl) {
	    output_lat_url_btree_range << lat << "\n";
	    output_mem_url_btree_range << mem << "\n";
	}
    }
}

void exec_group(const int expt_id, const bool is_point,
		int& expt_num, const int total_num_expt,
		const std::vector<std::string>& insert_emails,
		const std::vector<std::string>& insert_emails_sample,
		const std::vector<std::string>& txn_emails,
		const std::vector<std::string>& upper_bound_emails,
		const std::vector<std::string>& insert_wikis,
		const std::vector<std::string>& insert_wikis_sample,
		const std::vector<std::string>& txn_wikis,
		const std::vector<std::string>& upper_bound_wikis,
		const std::vector<std::string>& insert_urls,
		const std::vector<std::string>& insert_urls_sample,
		const std::vector<std::string>& txn_urls,
		const std::vector<std::string>& upper_bound_urls) {
    int dict_size[2] = {10000, 65536};
    std::cout << "-------------" << expt_num << "/" << total_num_expt << "--------------" << std::endl;
    exec(expt_id, kEmail, is_point, false, 0, 0,
	 insert_emails, insert_emails_sample, txn_emails, upper_bound_emails);
    expt_num++;

    std::cout << "-------------" << expt_num << "/" << total_num_expt << "--------------" << std::endl;
    exec(expt_id, kWiki, is_point, false, 0, 0,
	 insert_wikis, insert_wikis_sample, txn_wikis, upper_bound_wikis);
    expt_num++;

    std::cout << "-------------" << expt_num << "/" << total_num_expt << "--------------" << std::endl;
    exec(expt_id, kUrl, is_point, false, 0, 0,
	 insert_urls, insert_urls_sample, txn_urls, upper_bound_urls);
    expt_num++;
    //=================================================
    std::cout << "-------------" << expt_num << "/" << total_num_expt << "--------------" << std::endl;
    exec(expt_id, kEmail, is_point, true, 1, 1000,
	 insert_emails, insert_emails_sample, txn_emails, upper_bound_emails);
    expt_num++;

    std::cout << "-------------" << expt_num << "/" << total_num_expt << "--------------" << std::endl;
    exec(expt_id, kWiki, is_point, true, 1, 1000,
	 insert_wikis, insert_wikis_sample, txn_wikis, upper_bound_wikis);
    expt_num++;

    std::cout << "-------------" << expt_num << "/" << total_num_expt << "--------------" << std::endl;
    exec(expt_id, kUrl, is_point, true, 1, 1000,
	 insert_urls, insert_urls_sample, txn_urls, upper_bound_urls);
    expt_num++;
    //=================================================
    std::cout << "-------------" << expt_num << "/" << total_num_expt << "--------------" << std::endl;
    exec(expt_id, kEmail, is_point, true, 2, 65536,
	 insert_emails, insert_emails_sample, txn_emails, upper_bound_emails);
    expt_num++;

    std::cout << "-------------" << expt_num << "/" << total_num_expt << "--------------" << std::endl;
    exec(expt_id, kWiki, is_point, true, 2, 65536,
	 insert_wikis, insert_wikis_sample, txn_wikis, upper_bound_wikis);
    expt_num++;

    std::cout << "-------------" << expt_num << "/" << total_num_expt << "--------------" << std::endl;
    exec(expt_id, kUrl, is_point, true, 2, 65536,
	 insert_urls, insert_urls_sample, txn_urls, upper_bound_urls);
    expt_num++;
	    
    for (int j = 0; j < 2; j++) {
	std::cout << "-------------" << expt_num << "/" << total_num_expt << "--------------" << std::endl;
	exec(expt_id, kEmail, is_point, true, 3, dict_size[j],
	     insert_emails, insert_emails_sample, txn_emails, upper_bound_emails);
	expt_num++;

	std::cout << "-------------" << expt_num << "/" << total_num_expt << "--------------" << std::endl;
	exec(expt_id, kWiki, is_point, true, 3, dict_size[j],
	     insert_wikis, insert_wikis_sample, txn_wikis, upper_bound_wikis);
	expt_num++;

	std::cout << "-------------" << expt_num << "/" << total_num_expt << "--------------" << std::endl;
	exec(expt_id, kUrl, is_point, true, 3, dict_size[j],
	     insert_urls, insert_urls_sample, txn_urls, upper_bound_urls);
	expt_num++;
    }

    for (int j = 0; j < 2; j++) {
	std::cout << "-------------" << expt_num << "/" << total_num_expt << "--------------" << std::endl;
	exec(expt_id, kEmail, is_point, true, 4, dict_size[j],
	     insert_emails, insert_emails_sample, txn_emails, upper_bound_emails);
	expt_num++;

	std::cout << "-------------" << expt_num << "/" << total_num_expt << "--------------" << std::endl;
	exec(expt_id, kWiki, is_point, true, 4, dict_size[j],
	     insert_wikis, insert_wikis_sample, txn_wikis, upper_bound_wikis);
	expt_num++;

	std::cout << "-------------" << expt_num << "/" << total_num_expt << "--------------" << std::endl;
	exec(expt_id, kUrl, is_point, true, 4, dict_size[j],
	     insert_urls, insert_urls_sample, txn_urls, upper_bound_urls);
	expt_num++;
    }    
}

int main(int argc, char *argv[]) {
    int expt_id = (int)atoi(argv[1]);

    //-------------------------------------------------------------
    // Init Workloads
    //-------------------------------------------------------------
    std::vector<std::string> insert_emails, insert_emails_sample, txn_emails, upper_bound_emails;
    loadWorkload(kEmail, insert_emails, insert_emails_sample, txn_emails, upper_bound_emails);

    std::vector<std::string> insert_wikis, insert_wikis_sample, txn_wikis, upper_bound_wikis;
    loadWorkload(kWiki, insert_wikis, insert_wikis_sample, txn_wikis, upper_bound_wikis);

    std::vector<std::string> insert_urls, insert_urls_sample, txn_urls, upper_bound_urls;
    loadWorkload(kUrl, insert_urls, insert_urls_sample, txn_urls, upper_bound_urls);

    if (expt_id == 0) {
	//-------------------------------------------------------------
	// Point Queries; Expt ID = 0
	//-------------------------------------------------------------
	std::cout << "====================================" << std::endl;
	std::cout << "Point Queries; Expt ID = 0" << std::endl;
	std::cout << "====================================" << std::endl;

	output_lat_email_btree.open(file_lat_email_btree);
	output_mem_email_btree.open(file_mem_email_btree);

	output_lat_wiki_btree.open(file_lat_wiki_btree);
	output_mem_wiki_btree.open(file_mem_wiki_btree);

	output_lat_url_btree.open(file_lat_url_btree);
	output_mem_url_btree.open(file_mem_url_btree);

	bool is_point = true;
	int expt_num = 1;
	int total_num_expt = 21;
	exec_group(expt_id, is_point, expt_num, total_num_expt,
		   insert_emails, insert_emails_sample, txn_emails, upper_bound_emails,
		   insert_wikis, insert_wikis_sample, txn_wikis, upper_bound_wikis,
		   insert_urls, insert_urls_sample, txn_urls, upper_bound_urls);
	
	output_lat_email_btree.close();
	output_mem_email_btree.close();

	output_lat_wiki_btree.close();
	output_mem_wiki_btree.close();

	output_lat_url_btree.close();
	output_mem_url_btree.close();
    }
    else if (expt_id == 1) {
	//-------------------------------------------------------------
	// Range Queries; Expt ID = 1
	//-------------------------------------------------------------
	std::cout << "====================================" << std::endl;
	std::cout << "Range Queries; Expt ID = 1" << std::endl;
	std::cout << "====================================" << std::endl;

	output_lat_email_btree_range.open(file_lat_email_btree_range);
	output_mem_email_btree_range.open(file_mem_email_btree_range);

	output_lat_wiki_btree_range.open(file_lat_wiki_btree_range);
	output_mem_wiki_btree_range.open(file_mem_wiki_btree_range);

	output_lat_url_btree_range.open(file_lat_url_btree_range);
	output_mem_url_btree_range.open(file_mem_url_btree_range);

	bool is_point = false;
	int expt_num = 1;
	int total_num_expt = 21;
	exec_group(expt_id, is_point, expt_num, total_num_expt,
		   insert_emails, insert_emails_sample, txn_emails, upper_bound_emails,
		   insert_wikis, insert_wikis_sample, txn_wikis, upper_bound_wikis,
		   insert_urls, insert_urls_sample, txn_urls, upper_bound_urls);
	
	output_lat_email_btree_range.close();
	output_mem_email_btree_range.close();

	output_lat_wiki_btree_range.close();
	output_mem_wiki_btree_range.close();

	output_lat_url_btree_range.close();
	output_mem_url_btree_range.close();
    }

    return 0;
}
