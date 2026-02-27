#include <thread>
#include <iostream>
#include <stdlib.h>
#include <swss/events.h>
#include <swss/events_common.h>

/*
 * Sample i/p file contents for send
 *
 * {"src_0:key-0": {"foo": "bar", "hello": "world" }}
 * {"src_0:key-1": {"foo": "barXX", "hello": "world" }}
 *
 * Repeat the above line to increase entries.
 * Each line is parsed independently, so no "," expected at the end.
 */

#define ASSERT(res, m, ...) \
    if (!(res)) {\
        int _e = errno; \
        printf("Failed here %s:%d errno:%d zerrno:%d ", __FUNCTION__, __LINE__, _e, zmq_errno()); \
        printf(m, ##__VA_ARGS__); \
        printf("\n"); \
        exit(-1); }


typedef enum {
    OP_INIT=0,
    OP_SEND=1,
    OP_RECV=2,
    OP_SEND_RECV=3     //SEND|RECV
} op_t;


#define PRINT_CHUNK_SZ 2

/*
 * Usage:
 */

const char *s_usage = "\
-s  - To Send\n\
-r  - To receive\n\
Note:\n\
    when both -s & -r are given:\n\
        it uses main thread to publish and fork a dedicated thread to receive.\n\
        The rest of the parameters except -w is used for send\n\
\n\
-n  - Count of messages to send/receive. When both given, it is used as count to send\n\
      Default: 1 \n\
      A value of 0 implies unlimited\n\
\n\
-p  - Count of milliseconds to pause between sends or receives. In send-recv mode, it only affects send.\n\
      Default: 0 implying no pause\n\
\n\
      -i  - List of JSON messages to send in a file, with each event/message\n\
            declared in a single line. When n is more than size of list, the list\n\
            is rotated upon completion.\n\
      e.g. '[ \n\
                { \"sonic-bgp:bgp-state\": { \"ip\": \"10.101.01.10\", \"ts\": \"2022-10-11T01:02:30.45567\", \"state\": \"up\" }}\n\
                { \"abc-xxx:yyy-zz\": { \"foo\": \"bar\", \"hello\":\"world\", \"ts\": \"2022-10-11T01:02:30.45567\"}}\n\
                { \"some-mod:some-tag\": {}}\n\
            ]\n\
      Default: <some test message>\n\
\n\
-c  - Use offline cache in receive mode\n\
-o  - O/p file to write received events\n\
      Default: STDOUT\n";


bool term_receive = false;

template <typename Map>
string
t_map_to_str(const Map &m)
{
    stringstream _ss;
    string sep;

    _ss << "{";
    for (const auto &elem: m) {
        _ss << sep << "{" << elem.first << "," << elem.second << "}";
        if (sep.empty()) {
            sep = ", ";
        }
    }
    _ss << "}";
    return _ss.str();
}

void
do_receive(const event_subscribe_sources_t filter, const string outfile, int cnt, int pause, bool use_cache)
{
    int index=0, total_missed = 0;
    ostream* fp = &cout;
    ofstream fout;

    if (!outfile.empty()) {
        fout.open(outfile);
        if (!fout.fail()) {
            fp = &fout;
            printf("outfile=%s set\n", outfile.c_str());
        }
    }
    event_handle_t h = events_init_subscriber(use_cache, 2000, filter.empty() ? NULL : &filter);
    printf("Subscribed with use_cache=%d timeout=2000 filter %s\n",
            use_cache, filter.empty() ? "empty" : "non-empty");
    ASSERT(h != NULL, "Failed to get subscriber handle");

    while(!term_receive) {
        event_receive_op_t evt;
        map_str_str_t evtOp;

        int rc = event_receive(h, evt);
        if (rc != 0) {
            ASSERT(rc == EAGAIN, "Failed to receive rc=%d index=%d\n",
                    rc, index);
            continue;
        }
        ASSERT(!evt.key.empty(), "received EMPTY key");
        ASSERT(evt.missed_cnt >= 0, "Missed count uninitialized");
        ASSERT(evt.publish_epoch_ms > 0, "publish_epoch_ms uninitialized");

        total_missed += evt.missed_cnt;

        evtOp[evt.key] = t_map_to_str(evt.params);
        (*fp) << t_map_to_str(evtOp) << "\n";
        fp->flush();

        if ((++index % PRINT_CHUNK_SZ) == 0) {
            printf("Received index %d\n", index);
        }

        if (cnt > 0) {
            if (--cnt <= 0) {
                break;
            }
        }
    }

    events_deinit_subscriber(h);
    printf("Total received = %d missed = %dfile:%s\n", index, total_missed,
            outfile.empty() ? "STDOUT" : outfile.c_str());
}


int
do_send(const string infile, int cnt, int pause)
{
    typedef struct {
        string tag;
        event_params_t params;
    } evt_t;

    typedef vector<evt_t> lst_t;

    lst_t lst;
    string source;
    event_handle_t h;
    int index = 0;

    if (!infile.empty()) {
        ifstream input(infile);

        /* Read infile into list of events, that are ready for send */
        for( string line; getline( input, line ); )
        {
            evt_t evt;
            string str_params;

            const auto &data = nlohmann::json::parse(line);
            ASSERT(data.is_object(), "Parsed data is not object");
            ASSERT((int)data.size() == 1, "string parse size = %d", (int)data.size());

            string key(data.begin().key());
            if (source.empty()) {
                source = key.substr(0, key.find(":"));
            } else {
                ASSERT(source == key.substr(0, key.find(":")), "source:%s read=%s",
                        source.c_str(), key.substr(0, key.find(":")).c_str());
            }
            evt.tag = key.substr(key.find(":")+1);

            const auto &val = data.begin().value();
            ASSERT(val.is_object(), "Parsed params is not object");
            ASSERT((int)val.size() >= 1, "Expect non empty params");

            for(auto par_it = val.begin(); par_it != val.end(); par_it++) {
                evt.params[string(par_it.key())] = string(par_it.value());
            }
            lst.push_back(evt);
        }
    }

    if (lst.empty()) {
        evt_t evt = {
            "test-tag",
            {
                { "param1", "foo"},
                {"param2", "bar"}
            }
        };
        lst.push_back(evt);
    }

    h = events_init_publisher(source);
    ASSERT(h != NULL, "failed to init publisher");

    /* cnt = 0 as i/p implies forever */

    while(cnt >= 0) {
        /* Keep resending  the list until count is exhausted */
        for(lst_t::const_iterator itc = lst.begin(); (cnt >= 0) && (itc != lst.end()); ++itc) {
            const evt_t &evt = *itc;

            if ((++index % PRINT_CHUNK_SZ) == 0) {
                printf("Sending index %d\n", index);
            }

            int rc = event_publish(h, evt.tag, evt.params.empty() ? NULL : &evt.params);
            ASSERT(rc == 0, "Failed to publish index=%d rc=%d", index, rc);

            if ((cnt > 0) && (--cnt == 0)) {
                /* set to termninate */
                cnt = -1;
            }
            else if (pause) {
                /* Pause between two sends */
                this_thread::sleep_for(chrono::milliseconds(pause));
            }
        }
    }

    events_deinit_publisher(h);
    printf("Sent %d events\n", index);
    return 0;
}

void usage()
{
    printf("%s", s_usage);
    exit(-1);
}

int main(int argc, char **argv)
{
    bool use_cache = false;
    int op = OP_INIT;
    int cnt=0, pause=0;
    string json_str_msg, outfile("STDOUT"), infile;
    event_subscribe_sources_t filter;

    for(;;)
    {
        switch(getopt(argc, argv, "srn:p:i:o:f:c")) // note the colon (:) to indicate that 'b' has a parameter and is not a switch
        {
        case 'c':
            use_cache = true;
            continue;

        case 's':
            op |= OP_SEND;
            continue;

        case 'r':
            op |= OP_RECV;
            continue;

        case 'n':
            cnt = stoi(optarg);
            continue;

        case 'p':
            pause = stoi(optarg);
            continue;

        case 'i':
            infile = optarg;
            continue;

        case 'o':
            outfile = optarg;
            continue;

        case 'f':
            {
            stringstream ss(optarg); //create string stream from the string
            while(ss.good()) {
                string substr;
                getline(ss, substr, ',');
                filter.push_back(substr);
            }
            }
            continue;

        case -1:
            break;

        case '?':
        case 'h':
        default :
            usage();
            break;

        }
        break;
    }


    printf("op=%d n=%d pause=%d i=%s o=%s\n",
            op, cnt, pause, infile.c_str(), outfile.c_str());

    if (op == OP_SEND_RECV) {
        thread thr(&do_receive, filter, outfile, 0, 0, use_cache);
        do_send(infile, cnt, pause);
    }
    else if (op == OP_SEND) {
        do_send(infile, cnt, pause);
    }
    else if (op == OP_RECV) {
        do_receive(filter, outfile, cnt, pause, use_cache);
    }
    else {
        ASSERT(false, "Elect -s for send or -r receive or both; Bailing out with no action\n");
    }

    printf("--------- END: Good run -----------------\n");
    return 0;
}

