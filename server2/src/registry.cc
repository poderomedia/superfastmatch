#include "registry.h"

namespace superfastmatch{
	Registry::Registry(const string& filename){
		//Todo load config from file
		window_size=15;
		hash_width=32;
		thread_count=8;
		max_line_length=1<<16;
		max_hash_count=(1L<<32)-1;
		max_batch_count=20000;
		timeout=1.0;
		postings_path="/tmp/superfastmatch";
		documentDB = new kc::PolyDB();
		documentDB->open("document.kct#bnum=100000#zcomp=zlib");
		indexDB = new kc::TreeDB();
		// indexDB->open("index.kct#bnum=4000000#msiz=1g#fpow=16#psiz=32768#dfunit=64#pccap=256m#opts=lc");
		// // indexDB->tune_logger(&logger, kc::BasicDB::Logger::WARN |kc::BasicDB::Logger::ERROR);
		indexDB = new kc::TreeDB();
		indexDB->tune_options(kc::TreeDB::TLINEAR|kc::TreeDB::TCOMPRESS);
		indexDB->tune_buckets(4L*1000*1000);
		indexDB->tune_map(1LL<< 30);
		indexDB->tune_fbp(16);
		indexDB->tune_page(32768);
		indexDB->tune_defrag(64);
		indexDB->tune_page_cache(1LL<< 28);
		indexDB->open("index.kct");
		associationDB = new kc::PolyDB();
		associationDB->open("association.kct#bnum=100000");
		queueDB = new kc::PolyDB();
		queueDB->open("queue.kct#bnum=1000000#zcomp=zlib");
		miscDB = new kc::PolyDB();
		miscDB->open("misc.kch");
	}
	Registry::~Registry(){
		documentDB->close();
		indexDB->close();
		associationDB->close();
		queueDB->close();
		miscDB->close();
		delete documentDB;
		delete indexDB;
		delete associationDB;
		delete queueDB;
		delete miscDB;
	}
	
	std::ostream& operator<< (std::ostream& stream, Registry& registry) {
		registry.status(stream,registry.indexDB);
		return stream;
	}
	
	void Registry::status(std::ostream& s, kc::TreeDB* db){
	    std::map<std::string, std::string> status;
	    status["opaque"] = "";
	    status["fbpnum_used"] = "";
	    status["bnum_used"] = "";
	    status["cusage_lcnt"] = "";
	    status["cusage_lsiz"] = "";
	    status["cusage_icnt"] = "";
	    status["cusage_isiz"] = "";
	    status["tree_level"] = "";
		db->status(&status);
		uint32_t type = kc::atoi(status["realtype"].c_str());
      	oprintf(s,"type: %s (type=0x%02X) (%s)\n",
              status["type"].c_str(), type, kc::BasicDB::typestring(type));
      	uint32_t chksum = kc::atoi(status["chksum"].c_str());
      	oprintf(s,"format version: %s (libver=%s.%s) (chksum=0x%02X)\n", status["fmtver"].c_str(),
              status["libver"].c_str(), status["librev"].c_str(), chksum);
      	oprintf(s,"path: %s\n", status["path"].c_str());
      	int32_t flags = kc::atoi(status["flags"].c_str());
      	oprintf(s,"status flags:");
      	if (flags & kc::TreeDB::FOPEN) oprintf(s," open");
      	if (flags & kc::TreeDB::FFATAL) oprintf(s," fatal");
      	oprintf(s," (flags=%d)", flags);
      	if (kc::atoi(status["recovered"].c_str()) > 0) oprintf(s," (recovered)");
      	if (kc::atoi(status["reorganized"].c_str()) > 0) oprintf(s," (reorganized)");
      	if (kc::atoi(status["trimmed"].c_str()) > 0) oprintf(s," (trimmed)");
      	oprintf(s,"\n", flags);
      	int32_t apow = kc::atoi(status["apow"].c_str());
      	oprintf(s,"alignment: %d (apow=%d)\n", 1 << apow, apow);
      	int32_t fpow = kc::atoi(status["fpow"].c_str());
      	int32_t fbpnum = fpow > 0 ? 1 << fpow : 0;
      	int32_t fbpused = kc::atoi(status["fbpnum_used"].c_str());
      	int64_t frgcnt = kc::atoi(status["frgcnt"].c_str());
      	oprintf(s,"free block pool: %d (fpow=%d) (used=%d) (frg=%lld)\n",
              fbpnum, fpow, fbpused, (long long)frgcnt);
      	int32_t opts = kc::atoi(status["opts"].c_str());
      	oprintf(s,"options:");
      	if (opts & kc::TreeDB::TSMALL) oprintf(s," small");
      	if (opts & kc::TreeDB::TLINEAR) oprintf(s," linear");
      	if (opts & kc::TreeDB::TCOMPRESS) oprintf(s," compress");
      	oprintf(s," (opts=%d)\n", opts);
      	oprintf(s,"comparator: %s\n", status["rcomp"].c_str());
      	if (status["opaque"].size() >= 16) {
        	const char* opaque = status["opaque"].c_str();
        	oprintf(s,"opaque:");
        if (std::count(opaque, opaque + 16, 0) != 16) {
          for (int32_t i = 0; i < 16; i++) {
            oprintf(s," %02X", ((unsigned char*)opaque)[i]);
          }
        } else {
          oprintf(s," 0");
        }
        oprintf(s,"\n");
      	}
      	int64_t bnum = kc::atoi(status["bnum"].c_str());
      	int64_t bnumused = kc::atoi(status["bnum_used"].c_str());
      	int64_t count = kc::atoi(status["count"].c_str());
      	int64_t pnum = kc::atoi(status["pnum"].c_str());
      	int64_t lcnt = kc::atoi(status["lcnt"].c_str());
      	int64_t icnt = kc::atoi(status["icnt"].c_str());
      	int32_t tlevel = kc::atoi(status["tree_level"].c_str());
      	int32_t psiz = kc::atoi(status["psiz"].c_str());
      	double load = 0;
      	if (pnum > 0 && bnumused > 0) {
        	load = (double)pnum / bnumused;
        	if (!(opts & kc::TreeDB::TLINEAR)) load = std::log(load + 1) / std::log(2.0);
      	}
      	oprintf(s,"buckets: %lld (used=%lld) (load=%.2f)\n",
              (long long)bnum, (long long)bnumused, load);
      	oprintf(s,"pages: %lld (leaf=%lld) (inner=%lld) (level=%d) (psiz=%d)\n",
              (long long)pnum, (long long)lcnt, (long long)icnt, tlevel, psiz);
      	int64_t pccap = kc::atoi(status["pccap"].c_str());
      	int64_t cusage = kc::atoi(status["cusage"].c_str());
      	int64_t culcnt = kc::atoi(status["cusage_lcnt"].c_str());
      	int64_t culsiz = kc::atoi(status["cusage_lsiz"].c_str());
      	int64_t cuicnt = kc::atoi(status["cusage_icnt"].c_str());
      	int64_t cuisiz = kc::atoi(status["cusage_isiz"].c_str());
      	oprintf(s,"cache: %lld (cap=%lld) (ratio=%.2f) (leaf=%lld:%lld) (inner=%lld:%lld)\n",
              (long long)cusage, (long long)pccap, (double)cusage / pccap,
              (long long)culsiz, (long long)culcnt, (long long)cuisiz, (long long)cuicnt);
      	std::string cntstr = unitnumstr(count);
      	oprintf(s,"count: %lld (%s)\n", count, cntstr.c_str());
      	int64_t size = kc::atoi(status["size"].c_str());
      	int64_t msiz = kc::atoi(status["msiz"].c_str());
      	int64_t realsize = kc::atoi(status["realsize"].c_str());
      	std::string sizestr = unitnumstrbyte(size);
      	oprintf(s,"size: %lld (%s) (map=%lld)", size, sizestr.c_str(), (long long)msiz);
      	if (size != realsize) oprintf(s," (gap=%lld)", (long long)(realsize - size));
      	oprintf(s,"\n");
	}
}