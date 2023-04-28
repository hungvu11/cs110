#include <iostream>
#include <list>
#include <unordered_map>
#include <set>
#include <vector>
#include "imdb.h"

using namespace std;

static const int kWrongArgumentCount = 1;
static const int kDatabaseNotFound = 2;

int main(int argc, char *argv[]) {
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " <actor1> <actor2>" << endl;
        return kWrongArgumentCount;
    }

    imdb db(kIMDBDataDirectory);
    if (!db.good()) {
        cerr << "Data directory not found!  Aborting..." << endl;
        return kDatabaseNotFound;
    }

    return 0;
}
