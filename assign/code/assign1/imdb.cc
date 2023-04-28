#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <algorithm>
#include <fcntl.h>
#include <unistd.h>
#include "imdb.h"
using namespace std;

const char *const imdb::kActorFileName = "actordata";
const char *const imdb::kMovieFileName = "moviedata";
imdb::imdb(const string& directory) {
    const string actorFileName = directory + "/" + kActorFileName;
    const string movieFileName = directory + "/" + kMovieFileName;
    actorFile = acquireFileMap(actorFileName, actorInfo);
    movieFile = acquireFileMap(movieFileName, movieInfo);
}

bool imdb::good() const {
    return !( (actorInfo.fd == -1) || (movieInfo.fd == -1) );
}

imdb::~imdb() {
    releaseFileMap(actorInfo);
    releaseFileMap(movieInfo);
}

string imdb::getActorName(const int offset) const {
    return string((char *) actorFile + offset);
}

film imdb::getFilmData(const int offset) const {
    char *film_ptr = (char *) movieFile + offset;
    const string filmName = string(film_ptr);
    film_ptr += filmName.length() + 1;
    char year = *film_ptr;
    return film { filmName, year };
}

bool imdb::getCredits(const string& player, vector<film>& films) const {
    const int *countp = (const int *) actorFile;
    const int *begin = (const int *) actorFile + 1;
    const int *end = begin + *countp;
    const int *found = lower_bound(begin, end, player, [this](const int offset, const string& b) {
        return getActorName(offset).compare(b) < 0;
    });
    // if the player is not in the actorFile return false
    if (getActorName(*found).compare(player) != 0) return false;
    
    // Otherwise, proceed with the actor's record
    char* actorRecord = (char *) actorFile + *found;
    int numOfByte = 0;
    if (player.length() % 2) {
        numOfByte += (player.length() + 1);
    } else {
        numOfByte += (player.length() + 2);
    }
    actorRecord += numOfByte;
    short* listMovie = (short *) actorRecord;
    if (numOfByte % 4) actorRecord += 2;
    else actorRecord += 4;
        
    for (short i=0; i < *listMovie; i++) {
        int* movie = (int *) actorRecord;
        films.push_back(getFilmData(*movie));
        actorRecord += 4;
    }
    return true;
}

bool imdb::getCast(const film& movie, vector<string>& players) const {
    const int* numOfMovie = (int *) movieFile;
    const int* begin = (int *) movieFile + 1;
    const int* end = (int *) movieFile + *numOfMovie;
    const int* found = lower_bound(begin, end, movie, [this](const int offset, const film& b) {
        return getFilmData(offset) < b;
    }); 
    if (movie == getFilmData(*found)) {
        char* film = (char *) movieFile + *found;
        string filmName = string(film);
        // cout << filmName << '\n';
        int skipBytes = filmName.length() + 2;

        if (skipBytes % 2) skipBytes++;

        short* numOfActors = (short*) (film + skipBytes);
        // cout << *numOfActors << '\n';
        if (skipBytes % 4) skipBytes += 2;
        else skipBytes += 4;
        // cout << skipBytes << '\n';
        film += skipBytes;
        for (short i = 0; i<*numOfActors; i++) {
            int* filmN = (int *) film;
            players.push_back(getActorName(*filmN));
            film += 4;
        }
        return true;
    }
    return false;
}

const void *imdb::acquireFileMap(const string& fileName, struct fileInfo& info) {
    struct stat stats;
    stat(fileName.c_str(), &stats);
    info.fileSize = stats.st_size;
    info.fd = open(fileName.c_str(), O_RDONLY);
    return info.fileMap = mmap(0, info.fileSize, PROT_READ, MAP_SHARED, info.fd, 0);
}

void imdb::releaseFileMap(struct fileInfo& info) {
    if (info.fileMap != NULL) munmap((char *) info.fileMap, info.fileSize);
    if (info.fd != -1) close(info.fd);
}
