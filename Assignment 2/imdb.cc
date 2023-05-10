using namespace std;
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "imdb.h"

const char *const imdb::kActorFileName = "actordata";
const char *const imdb::kMovieFileName = "moviedata";

imdb::imdb(const string& directory)
{
  const string actorFileName = directory + "/" + kActorFileName;
  const string movieFileName = directory + "/" + kMovieFileName;
  
  actorFile = acquireFileMap(actorFileName, actorInfo);
  movieFile = acquireFileMap(movieFileName, movieInfo);
}

bool imdb::good() const
{
  return !( (actorInfo.fd == -1) || 
	    (movieInfo.fd == -1) ); 
}

film imdb::getFilm(int offset) const{
    film result;
    char* ptr = (char*)movieFile + offset;
    for(; *ptr != '\0'; ptr++){
        result.title += *ptr;
    }
    ptr++; // moving to the year
    result.year = 1900 + *(char*)ptr;
    return result;
}

string imdb::getActor(int offset) const{
    char* ptr = (char*)actorFile + offset;
    string result;
    for(; *ptr != '\0'; ptr++){
        result += *ptr;
    }
    return result;
}

void imdb::getMoviesForActor(vector<film> & films, char* actorInfo, const string& player) const{
    int nameSize = 1 + player.length(); // null terminated included ! '/0'
    int offset = nameSize; // counting bytes
    /*for(; *(actorInfo + offset) != '\0'; offset++){
        nameSize++;
    }*/
    // a b c d /0
    if(offset % 2 != 0){
        nameSize++;
        offset++;
    }
    int movieCount = *(short*)(actorInfo + offset);
    offset += 2;
    if((offset) % 4 != 0){
        offset += 2;
    }
    
    for(int i = 0; i < movieCount; i++){
        int movieOffset = *((int*)(actorInfo + offset) + i);
        films.push_back(getFilm(movieOffset));
    }
}

void imdb::getActorsForMovie(vector<string>& players, char* movieInfoStart, const film& movie) const{
    int nameSize = 1 + movie.title.length();
    int offset = nameSize + 1; // add one byte for year :O
    if(offset % 2 != 0){
        offset++;
        nameSize++;
    }
    short actorAmount = *(short*)(movieInfoStart + offset);
    offset+=2;
    if(offset % 4 != 0){
        offset += 2;
    }
    for(int i = 0; i < actorAmount; i++){
        int actorOffset = *((int*)(movieInfoStart + offset) + i);
        players.push_back(getActor(actorOffset));
    }
}

struct actorKey{
    const char* name;
    const void* fileName;   
};

struct movieKey{
    film* movie;
    const void* fileName;
};

int actcmp(const void* ptr1, const void* ptr2) {
    actorKey* key = (actorKey*)ptr1;
    const char* compName = (char*)key->fileName + *(int*)ptr2;
    const char* actorName = key->name;
    return strcmp(actorName, compName);
}

int moviecmp(const void* ptr1, const void* ptr2) {
    movieKey* key = (movieKey*)ptr1;
    film toComp;
    string title;
    char* ptr = (char*)key->fileName + *(int*)ptr2;
    for(; *ptr != '\0'; ptr++){
        title += *ptr;
    }
    /*while(*ptr != '\0'){
        title += *ptr;
        ptr++;
    }*/
    ptr++;
    int year = 1900 + *(char*)ptr;
    toComp.title = title;
    toComp.year = year;
    if(*key->movie < toComp){
        return -1;
    }else if(*key->movie == toComp){
        return 0;
    }else{
        return 1;
    }
}

bool imdb::getCredits(const string& player, vector<film>& films) const{ 
    actorKey myKey;
    int numberOfActors = *(int*)actorFile;
    int* arrStart = (int*)actorFile + 1;
    char* actorName = (char*)player.c_str();
    myKey.fileName = actorFile;
    myKey.name = actorName;
    int* actorOffsetAdress = (int*)bsearch(&myKey, arrStart, numberOfActors, sizeof(int), actcmp); 
    if(actorOffsetAdress == NULL) {
        return false;
    }
    char* actorInfoStart = (char*)actorFile + *actorOffsetAdress;
    getMoviesForActor(films, actorInfoStart, player);
    /*for(int* ptr = arrStart; ptr < arrStart + numberOfActors; ptr++){
        int adressOffset = *ptr;
        char* actorInfo = (char*)actorFile + adressOffset;
        if(strcmp(actorInfo, player.c_str()) == 0){
            getMoviesForActor(films, actorInfo);
        }
    }*/
    return true;
}

bool imdb::getCast(const film& movie, vector<string>& players) const { 
    movieKey myKey;
    int numberOfMovies = *(int*)movieFile;
    int* arrStart = (int*)movieFile + 1;
    film* myPtr = (film*)&movie;
    myKey.fileName = movieFile;
    myKey.movie = myPtr;
    int* movieOffsetAdress = (int*)bsearch(&myKey, arrStart, numberOfMovies, sizeof(int), moviecmp);
    if(movieOffsetAdress == NULL){
        return false;
    } 
    char* movieInfoStart = (char*)movieFile + *movieOffsetAdress;
    getActorsForMovie(players, movieInfoStart, movie);
    return true;
}

imdb::~imdb()
{
  releaseFileMap(actorInfo);
  releaseFileMap(movieInfo);
}

// ignore everything below... it's all UNIXy stuff in place to make a file look like
// an array of bytes in RAM.. 
const void *imdb::acquireFileMap(const string& fileName, struct fileInfo& info)
{
  struct stat stats;
  stat(fileName.c_str(), &stats);
  info.fileSize = stats.st_size;
  info.fd = open(fileName.c_str(), O_RDONLY);
  return info.fileMap = mmap(0, info.fileSize, PROT_READ, MAP_SHARED, info.fd, 0);
}

void imdb::releaseFileMap(struct fileInfo& info)
{
  if (info.fileMap != NULL) munmap((char *) info.fileMap, info.fileSize);
  if (info.fd != -1) close(info.fd);
}
