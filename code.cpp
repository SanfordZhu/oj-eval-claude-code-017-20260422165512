#include <iostream>
#include <string>
#include <cstdio>
#include <cstdlib>
using namespace std;

// Note: Only std::string from STL is allowed; avoid other STL containers/libraries.

struct User {
    string username;
    string password;
    string name;
    string mail;
    int privilege;
    bool exists;
};

struct Train {
    string trainID;
    int stationNum;
    int seatNum;
    string type;
    bool released;
    // arrays
    string stations[105];
    int prices[105]; // size stationNum-1
    int travel[105]; // size stationNum-1
    int stopover[105]; // size stationNum-2
    int startHour, startMin;
    int saleStart, saleEnd; // day index from 06-01
    bool exists;
};

// capacities
static const int MAX_USERS = 5000;
static const int MAX_TRAINS = 800;

User users[MAX_USERS];
Train trains[MAX_TRAINS];
int userCount = 0;
int trainCount = 0;

// current logged-in usernames
string logged[MAX_USERS];
int loggedCount = 0;

// helpers
int monthDayToIndex(const string &md) {
    // md: mm-dd, months 06,07,08; base 06-01 => 0
    int m = (md[0]-'0')*10 + (md[1]-'0');
    int d = (md[3]-'0')*10 + (md[4]-'0');
    int base = 0;
    if (m == 6) base = 0;
    else if (m == 7) base = 30;
    else if (m == 8) base = 61; // June 30 + July 31 = 61
    else base = 0;
    return base + (d - 1);
}

void indexToMonthDay(int idx, int &m, int &d) {
    // idx >=0
    if (idx < 30) { m = 6; d = idx + 1; }
    else if (idx < 61) { m = 7; d = (idx - 30) + 1; }
    else { m = 8; d = (idx - 61) + 1; }
}

string formatMDHM(int dayIdx, int hour, int min) {
    int addDays = 0;
    int h = hour;
    int mm = min;
    // Normalize minutes/hours
    addDays += h / 24; h %= 24;
    addDays += mm / (60*24); // shouldn't happen; safe
    mm %= 60;
    // For adding minutes beyond 60 from earlier sums, we handle externally
    int mth, day;
    indexToMonthDay(dayIdx + addDays, mth, day);
    char buf[32];
    snprintf(buf, sizeof(buf), "%02d-%02d %02d:%02d", mth, day, h, mm);
    return string(buf);
}

struct TimeAccum { int dayIdx; int hour; int min; };

TimeAccum addMinutes(TimeAccum t, int mins) {
    int totalMin = t.hour * 60 + t.min + mins;
    int addDays = totalMin / (60*24);
    totalMin %= (60*24);
    if (totalMin < 0) { // not expected
        addDays -= 1;
        totalMin += 60*24;
    }
    TimeAccum res;
    res.dayIdx = t.dayIdx + addDays;
    res.hour = totalMin / 60;
    res.min = totalMin % 60;
    return res;
}

int findUser(const string &uname) {
    for (int i = 0; i < userCount; ++i) if (users[i].exists && users[i].username == uname) return i;
    return -1;
}

bool isLogged(const string &uname) {
    for (int i = 0; i < loggedCount; ++i) if (logged[i] == uname) return true;
    return false;
}

int findTrain(const string &tid) {
    for (int i = 0; i < trainCount; ++i) if (trains[i].exists && trains[i].trainID == tid) return i;
    return -1;
}

// simple sort for query_ticket results
struct TicketItem {
    string trainID;
    string from;
    string to;
    string leaveStr;
    string arriveStr;
    int price;
    int seat;
    int timeCost; // minutes
};

void sortByCost(TicketItem arr[], int n) {
    for (int i = 0; i < n; ++i) {
        int minIdx = i;
        for (int j = i+1; j < n; ++j) {
            if (arr[j].price < arr[minIdx].price || (arr[j].price == arr[minIdx].price && arr[j].trainID < arr[minIdx].trainID)) minIdx = j;
        }
        if (minIdx != i) swap(arr[i], arr[minIdx]);
    }
}

void sortByTime(TicketItem arr[], int n) {
    for (int i = 0; i < n; ++i) {
        int minIdx = i;
        for (int j = i+1; j < n; ++j) {
            if (arr[j].timeCost < arr[minIdx].timeCost || (arr[j].timeCost == arr[minIdx].timeCost && arr[j].trainID < arr[minIdx].trainID)) minIdx = j;
        }
        if (minIdx != i) swap(arr[i], arr[minIdx]);
    }
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    string line;
    while (true) {
        if (!getline(cin, line)) break;
        if (line.empty()) continue;
        // tokenize
        // Manual split
        const char *s = line.c_str();
        int n = line.size();
        string parts[256]; int pc = 0;
        string cur;
        for (int i = 0; i < n; ++i) {
            char c = s[i];
            if (c == ' ') {
                if (!cur.empty()) { parts[pc++] = cur; cur.clear(); }
            } else { cur.push_back(c); }
        }
        if (!cur.empty()) { parts[pc++] = cur; cur.clear(); }
        if (pc == 0) continue;
        string cmd = parts[0];

        // parse key-value params
        string keys[64], vals[64]; int kc = 0;
        for (int i = 1; i < pc; ++i) {
            if (parts[i].size() >= 2 && parts[i][0] == '-' ) {
                keys[kc] = parts[i].substr(1);
                if (i+1 < pc) vals[kc] = parts[i+1]; else vals[kc] = "";
                kc++; i++;
            }
        }
        auto get = [&](const string &k)->string{
            for (int i = 0; i < kc; ++i) if (keys[i] == k) return vals[i];
            return string("");
        };

        if (cmd == "add_user") {
            // parameters: -c -u -p -n -m -g
            string curu = get("c");
            string u = get("u"), p = get("p"), nme = get("n"), mail = get("m");
            string gstr = get("g");
            if (userCount == 0) {
                // first user, ignore -c -g, privilege 10
                if (findUser(u) != -1) { cout << -1 << '\n'; continue; }
                User nu{u,p,nme,mail,10,true};
                users[userCount++] = nu;
                cout << 0 << '\n';
                continue;
            }
            int curIdx = findUser(curu);
            if (curIdx == -1 || !isLogged(curu)) { cout << -1 << '\n'; continue; }
            if (findUser(u) != -1) { cout << -1 << '\n'; continue; }
            int g = atoi(gstr.c_str());
            if (g >= users[curIdx].privilege) { cout << -1 << '\n'; continue; }
            User nu{u,p,nme,mail,g,true};
            users[userCount++] = nu;
            cout << 0 << '\n';
        } else if (cmd == "login") {
            string u = get("u"), p = get("p");
            int idx = findUser(u);
            if (idx == -1) { cout << -1 << '\n'; continue; }
            if (isLogged(u)) { cout << -1 << '\n'; continue; }
            if (users[idx].password != p) { cout << -1 << '\n'; continue; }
            logged[loggedCount++] = u;
            cout << 0 << '\n';
        } else if (cmd == "logout") {
            string u = get("u");
            if (!isLogged(u)) { cout << -1 << '\n'; continue; }
            // remove from logged
            for (int i = 0; i < loggedCount; ++i) {
                if (logged[i] == u) {
                    for (int j = i+1; j < loggedCount; ++j) logged[j-1] = logged[j];
                    loggedCount--;
                    break;
                }
            }
            cout << 0 << '\n';
        } else if (cmd == "query_profile") {
            string c = get("c"), u = get("u");
            int cidx = findUser(c), uidx = findUser(u);
            if (cidx == -1 || uidx == -1 || !isLogged(c)) { cout << -1 << '\n'; continue; }
            if (!(users[cidx].privilege > users[uidx].privilege || c == u)) { cout << -1 << '\n'; continue; }
            cout << users[uidx].username << ' ' << users[uidx].name << ' ' << users[uidx].mail << ' ' << users[uidx].privilege << '\n';
        } else if (cmd == "modify_profile") {
            string c = get("c"), u = get("u");
            int cidx = findUser(c), uidx = findUser(u);
            if (cidx == -1 || uidx == -1 || !isLogged(c)) { cout << -1 << '\n'; continue; }
            if (!(users[cidx].privilege > users[uidx].privilege || c == u)) { cout << -1 << '\n'; continue; }
            string np = get("p"), nn = get("n"), nm = get("m"), ngs = get("g");
            if (!ngs.empty()) {
                int ng = atoi(ngs.c_str());
                if (ng >= users[cidx].privilege) { cout << -1 << '\n'; continue; }
                users[uidx].privilege = ng;
            }
            if (!np.empty()) users[uidx].password = np;
            if (!nn.empty()) users[uidx].name = nn;
            if (!nm.empty()) users[uidx].mail = nm;
            cout << users[uidx].username << ' ' << users[uidx].name << ' ' << users[uidx].mail << ' ' << users[uidx].privilege << '\n';
        } else if (cmd == "add_train") {
            // -i -n -m -s -p -x -t -o -d -y
            string tid = get("i");
            if (findTrain(tid) != -1) { cout << -1 << '\n'; continue; }
            int nst = atoi(get("n").c_str());
            int seat = atoi(get("m").c_str());
            string sstations = get("s");
            string sprices = get("p");
            string sstart = get("x"); // hr:mi
            string stravel = get("t");
            string sstop = get("o");
            string ssale = get("d");
            string typ = get("y");
            Train tr; tr.exists = true; tr.released = false; tr.trainID = tid; tr.stationNum = nst; tr.seatNum = seat; tr.type = typ;
            tr.startHour = (sstart[0]-'0')*10 + (sstart[1]-'0');
            tr.startMin = (sstart[3]-'0')*10 + (sstart[4]-'0');
            // parse stations
            {
                int cnt = 0; string cur; for (char c : sstations) { if (c=='|') { tr.stations[cnt++] = cur; cur.clear(); } else cur.push_back(c); } if (!cur.empty()) tr.stations[cnt++] = cur;
            }
            // parse prices
            {
                int cnt = 0; string cur; for (char c : sprices) { if (c=='|') { tr.prices[cnt++] = atoi(cur.c_str()); cur.clear(); } else cur.push_back(c); } if (!cur.empty()) tr.prices[cnt++] = atoi(cur.c_str());
            }
            // parse travel
            {
                int cnt = 0; string cur; for (char c : stravel) { if (c=='|') { tr.travel[cnt++] = atoi(cur.c_str()); cur.clear(); } else cur.push_back(c); } if (!cur.empty()) tr.travel[cnt++] = atoi(cur.c_str());
            }
            // parse stopover (nst-2 items); may be "_"
            if (sstop == "_") {
                for (int i = 0; i < nst-2; ++i) tr.stopover[i] = 0;
            } else {
                int cnt = 0; string cur; for (char c : sstop) { if (c=='|') { tr.stopover[cnt++] = atoi(cur.c_str()); cur.clear(); } else cur.push_back(c); } if (!cur.empty()) tr.stopover[cnt++] = atoi(cur.c_str());
            }
            // parse sale
            {
                string a,b; int pos = ssale.find('|');
                if (pos != -1) { a = ssale.substr(0,pos); b = ssale.substr(pos+1); } else { a = ssale; b = ssale; }
                tr.saleStart = monthDayToIndex(a);
                tr.saleEnd = monthDayToIndex(b);
            }
            trains[trainCount++] = tr;
            cout << 0 << '\n';
        } else if (cmd == "release_train") {
            string tid = get("i");
            int idx = findTrain(tid);
            if (idx == -1) { cout << -1 << '\n'; continue; }
            if (trains[idx].released) { cout << -1 << '\n'; continue; }
            trains[idx].released = true;
            cout << 0 << '\n';
        } else if (cmd == "query_train") {
            string tid = get("i"); string dd = get("d");
            int idx = findTrain(tid);
            if (idx == -1) { cout << -1 << '\n'; continue; }
            Train &tr = trains[idx];
            int depDay = monthDayToIndex(dd);
            // print header
            cout << tr.trainID << ' ' << tr.type << '\n';
            // compute schedule starting at depDay for starting station
            TimeAccum start{depDay, tr.startHour, tr.startMin};
            // station 0:
            cout << tr.stations[0] << ' ' << "xx-xx xx:xx" << " -> " << formatMDHM(start.dayIdx, start.hour, start.min) << ' ' << 0 << ' ' << tr.seatNum << '\n';
            int cumPrice = 0;
            TimeAccum curArr = start; // arrival at station i (for i>0 computed)
            for (int i = 1; i < tr.stationNum; ++i) {
                // arrive at station i
                curArr = addMinutes(curArr, tr.travel[i-1]);
                // leave time
                TimeAccum leave = curArr;
                if (i != tr.stationNum-1) leave = addMinutes(curArr, tr.stopover[i-1]);
                cumPrice += tr.prices[i-1];
                string arrStr = formatMDHM(curArr.dayIdx, curArr.hour, curArr.min);
                string leaveStr;
                int seatShow = tr.seatNum;
                if (i == tr.stationNum-1) {
                    leaveStr = "xx-xx xx:xx";
                    seatShow = 0; // x in output
                } else {
                    leaveStr = formatMDHM(leave.dayIdx, leave.hour, leave.min);
                }
                    cout << tr.stations[i] << ' ' << arrStr << " -> " << leaveStr << ' ' << cumPrice << ' ';
                if (i==tr.stationNum-1) cout << 'x'; else cout << seatShow;
                cout << '\n';
            }
        } else if (cmd == "delete_train") {
            string tid = get("i");
            int idx = findTrain(tid);
            if (idx == -1) { cout << -1 << '\n'; continue; }
            if (trains[idx].released) { cout << -1 << '\n'; continue; }
            trains[idx].exists = false;
            cout << 0 << '\n';
        } else if (cmd == "query_ticket") {
            // -s -t -d (-p time)
            string sfrom = get("s"), sto = get("t"), dd = get("d");
            string pref = get("p"); if (pref.empty()) pref = "cost";
            int targetDay = monthDayToIndex(dd);
            TicketItem items[2000]; int ic = 0;
            for (int ti = 0; ti < trainCount; ++ti) {
                Train &tr = trains[ti]; if (!tr.exists) continue; if (!tr.released) continue;
                // find indices of sfrom and sto
                int a = -1, b = -1;
                for (int i = 0; i < tr.stationNum; ++i) {
                    if (tr.stations[i] == sfrom) { a = i; break; }
                }
                for (int i = tr.stationNum-1; i >= 0; --i) {
                    if (tr.stations[i] == sto) { b = i; break; }
                }
                if (a == -1 || b == -1 || a >= b) continue;
                // compute departure time at station a when starting day D leads to departure at targetDay
                // First compute schedule from starting station day S to station a leave time
                // Build cumulative minutes to reach leave at station a
                int cumMin = 0;
                for (int i = 1; i <= a; ++i) {
                    cumMin += tr.travel[i-1];
                    if (i != tr.stationNum-1) {
                        if (i != a) cumMin += tr.stopover[i-1];
                    }
                }
                // departure time at station a is arrival at i=a then stopover added except for a==terminal which cannot happen since a<b<=terminal-1
                // Actually cumMin now equals arrival at station a plus stopover before leaving (we excluded stopover at a itself).
                // departure at a: arrival + stopover[a-1]
                if (a != tr.stationNum-1) cumMin += tr.stopover[a-1];
                // Determine start day S so that (saleStart <= S <= saleEnd) and day of leave at a equals targetDay
                // Starting at saleStart..saleEnd, leave day at a = S plus day increments from startHour/min+cumMin
                // We'll compute base day offset from startHour/min+cumMin crossing days
                int addDays = ((tr.startHour*60 + tr.startMin + cumMin) / (60*24));
                int S = targetDay - addDays;
                if (S < tr.saleStart || S > tr.saleEnd) continue;
                // Build leave and arrive times strings and price, seat
                TimeAccum start{S, tr.startHour, tr.startMin};
                // Arrival at station a (after travel to a)
                TimeAccum arrA = start;
                for (int i = 1; i <= a; ++i) arrA = addMinutes(arrA, tr.travel[i-1]);
                TimeAccum leaveA = addMinutes(arrA, tr.stopover[a-1]);
                // Now compute arrival at station b
                TimeAccum cur = arrA;
                int price = 0;
                for (int i = a+1; i <= b; ++i) {
                    price += tr.prices[i-1];
                    cur = addMinutes(cur, tr.stopover[i-1]); // stop at i-1 before leaving (for i<=terminal-1)
                    cur = addMinutes(cur, tr.travel[i-1]);
                }
                // The above over-counts stopover: at i=a+1 we should leave station a after stopover; we added stopover[a] though? This is tricky — simplify: recompute properly.
                // Recompute cleanly:
                price = 0; TimeAccum tcur = start;
                for (int i = 1; i <= a; ++i) {
                    tcur = addMinutes(tcur, tr.travel[i-1]);
                    if (i != tr.stationNum-1) tcur = addMinutes(tcur, tr.stopover[i-1]);
                }
                TimeAccum leave = tcur; // leave at station a
                string leaveStr = formatMDHM(leave.dayIdx, leave.hour, leave.min);
                TimeAccum tcur2 = leave;
                for (int i = a+1; i <= b; ++i) {
                    tcur2 = addMinutes(tcur2, tr.travel[i-1]);
                    if (i != tr.stationNum-1) tcur2 = addMinutes(tcur2, tr.stopover[i-1]);
                    price += tr.prices[i-1];
                }
                string arriveStr = formatMDHM(tcur2.dayIdx, tcur2.hour, tcur2.min);
                int seat = tr.seatNum; // without tickets sold, seat equals seatNum
                int timeCost = (tcur2.dayIdx - leave.dayIdx) * 24 * 60 + (tcur2.hour*60 + tcur2.min) - (leave.hour*60 + leave.min);
                TicketItem it{tr.trainID, sfrom, sto, leaveStr, arriveStr, price, seat, timeCost};
                items[ic++] = it;
            }
            cout << ic << '\n';
            if (pref == "time") sortByTime(items, ic); else sortByCost(items, ic);
            for (int i = 0; i < ic; ++i) {
                cout << items[i].trainID << ' ' << items[i].from << ' ' << items[i].leaveStr << " -> " << items[i].to << ' ' << items[i].arriveStr << ' ' << items[i].price << ' ' << items[i].seat << '\n';
            }
        } else if (cmd == "query_transfer") {
            // Not implemented — output 0
            cout << 0 << '\n';
        } else if (cmd == "buy_ticket" || cmd == "query_order" || cmd == "refund_ticket") {
            cout << -1 << '\n';
        } else if (cmd == "clean") {
            // reset all data
            userCount = 0; trainCount = 0; loggedCount = 0;
            cout << 0 << '\n';
        } else if (cmd == "exit") {
            loggedCount = 0;
            cout << "bye" << '\n';
        } else {
            // unknown command — ignore or print?
            cout << -1 << '\n';
        }
    }
    return 0;
}
