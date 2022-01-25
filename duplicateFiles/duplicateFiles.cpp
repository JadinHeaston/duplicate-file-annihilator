// duplicateFiles.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

//LIBRARIES
#include <iostream> //Yeah.
#include <boost/filesystem.hpp> //Used for getting file date metadata (date modified and date created). Struggled to switch to the built in C++ filesystem for this.
#include <chrono> //Time tracking.
#include <filesystem> //Great for ease and function overloading things that allow wide character sets compared to Boost.
#include <fstream> //File input/output.
#include <openssl/md5.h> //Used for MD5 hashing. Pretty good!
#include <string> //Strings!
#include <sstream> //Stringstream
#include "thread_pool.hpp" //Thread pool stuff.
#include <map> //Map.
#include <vector> //vector.



//Sets global delimiter used for reading and writing DB files. Tilde typically works well. (CONSIDER USING MULTIPLE CHARACTER DELIMITER FOR SAFETY)
std::wstring delimitingCharacter = L"▼";
//Simple newline dude.
std::wstring newLine = L"\n";
//Buffer size for hashing
const int hashBufferSize = 4096;

bool recursiveSearch = true; //Recieved from arg: --no-recursive | defaults to true.

//Holds provided directory given from args. "Super Parents"
std::wstring givenDirectoryPath;


//Creating threadpools.
thread_pool threadPool(std::thread::hardware_concurrency()); //"Default" pool

thread_pool hashingThreadPool(std::thread::hardware_concurrency());


std::ofstream outputFile; //Hold potential future file handle if verbose debugging is enabled.
std::wstring outputFilePath = L"";
std::wstring outputFileName = L"debug.txt";
int outputFileCount = 1;

//Counters
size_t deletedFileCount = 0;
size_t duplicateFileCount = 0;
size_t uniqueFileCount = 0;

int main(int argc, char* argv[])
{
    //START TIMER.
    std::chrono::time_point start = std::chrono::steady_clock::now();

    //Verifying that no \ escaped " exist in the path string.
    for (int i = 0; i < argc; i++)
    {
        std::size_t found = std::string(argv[i]).find("\"");
        if (found != std::string::npos)
        {
            std::cout << "ERROR: Rogue quote found. Likely due to a \"\\\" placed before a double quote (\"). Please double check your input and try again." << std::endl;
            system("PAUSE");
            return 0;
        }
    }

    //Reading args
    if (argc == 1) //No arguments provided. Notify. Close program.
    {
        std::cout << "No arguments provided.\nUse the \"-h\" or \"--help\" switch to show the available options.\n(-s and -d are required for operation)" << std::endl;
        system("PAUSE");
        return 0;
    }
    else if (argc == 2) //Checking for help message.
    {
        if (strncmp(argv[1], "-h", 3) == 0 || strcmp(argv[1], "--help") == 0) //Checking second argument for if it is "-h" or "-help".
        {
            //Display help
            std::cout << "Defaults:" << std::endl;
            std::cout << "--output-verbose-debug <FILEPATH> - NULL | --no-recursive - T" << std::endl;
            std::cout << "HELP PROVIDED. GET FUCKED" << std::endl;

            system("PAUSE");
            return 0;
        }
        else //No arguments provided. Notify. Close program.
        {
            std::cout << "Use the \"-h\" or \"--help\" switch to show the available options.\n(-s and -d are required for operation)" << std::endl;
            system("PAUSE");
            return 0;
        }
    }

    for (int i = 0; i < argc; i++) // Cycle through all arguments.
    {
        if (strncmp(argv[i], "-s", 2) == 0) //Source path switch.
        {
            givenDirectoryPath = formatFilePath(charToWString(argv[i + 1]));

            if (givenDirectoryPath.back() == L'\\')
                givenDirectoryPath.pop_back(); //Remove the slash.

            if (!std::filesystem::is_directory(givenDirectoryPath)) //Verify path is real and valid.
            {
                std::wcout << "-s path provided was NOT found. (" << givenDirectoryPath << ")" << std::endl;
                system("PAUSE");
                return 0;
            }
        }
        else if (strncmp(argv[i], "--no-recursive", 32) == 0) //Disable recursive operation.
            recursiveSearch = false;
    }

    //***** NEEDS COMMENTING
    if (outputFilePath.find(L"/"))
    {
        outputFileName = outputFilePath.substr(outputFilePath.find_last_of(L"/") + 1, std::wstring::npos);

        outputFilePath = outputFilePath.substr(0, outputFilePath.find_last_of(L"/") + 1); //Remove filename from path.
    }
    else
    {
        outputFileName = outputFilePath;
        outputFilePath = L"";
    }

    outputFile.open(outputFilePath + outputFileName, std::ios::out | std::ios::binary | std::ios::app); //Open the file.
    if (!outputFile.is_open())
    {
        std::wcout << L"Debug file path not usable: " + outputFilePath + outputFileName << std::endl;
        system("PAUSE");
        return 0;
    }
    outputFile.close();

	//Creating vectors to hold directory maps.
	std::vector<std::wstring> directoryDB;
    //File action vector
    std::vector<std::wstring> fileOpActions;

    //MAX_PATH bypass.
    //Also ensuring that path is an absolute path.
    givenDirectoryPath = formatFilePath(L"//?/" + std::filesystem::absolute(givenDirectoryPath).wstring());
   

    //Double check that there is no slash.
    //This was added because running a drive letter ("D:") through absolute adds one.

    if (givenDirectoryPath.back() == L'/')
        givenDirectoryPath.pop_back(); //Remove the slash.

    //Creating directory map.
    std::cout << "Creating directory list..." << std::endl; //***
    threadPool.push_task(createDirectoryMapDB, std::ref(directoryDB), std::ref(givenDirectoryPath));
    threadPool.wait_for_tasks();
    std::cout << "Directory list created..." << std::endl; //***


    //Semi-Sorting directories. This may be changed to be a natural sorting later. (to make it more human-readable)
    std::cout << "Sorting lists..." << std::endl; //***
    threadPool.push_task(sortDirectoryDatabases, std::ref(directoryDB));
    threadPool.wait_for_tasks();
    std::cout << "Sorting finished..." << std::endl; //***

    //Compare files. Determine what files need to be hashed.



    //Start hashing files.
    std::cout << "Beginning hash process. " << directoryDB.size() << " Files to be hashed..." << std::endl; //***
    assignHashTasks(directoryDB, givenDirectoryPath); //Assigning hash tasks
    std::cout << "Waiting for hashing to finish..." << std::endl;
    hashingThreadPool.wait_for_tasks(); //Waiting for hashing to finish.
    std::cout << "Hashing finished!" << std::endl; //***
    std::cout << "Comparing file hashes..." << std::endl; ///***
    compareHashes(fileOpActionFile, givenDirectoryDB, givenDirectoryPath);
    std::cout << "Hash comparison finished!" << std::endl; //***

    //If we are deleting files, do so.

    //Create 















    //OUTPUTTING FILES:
    // 
    // 
    // 
    
    //Add headers to directory file.
    std::wstring columnsLine = L"PATH" + delimitingCharacter + L"FILE_SIZE" + delimitingCharacter + L"MD5_HASH" + delimitingCharacter + newLine;
    //Rotate all elements to the right two, to allow the header and path information to be at the top/start.
    std::rotate(directoryDB.rbegin(), directoryDB.rbegin() + 2, directoryDB.rend());

    //Holds output of hashAction reading, allows for manipulation.
    std::wstring currentReadLine;

    //Directory list file.
    std::wstring firstDirectoryDB = L"directoryList.txt";
    std::ofstream firstFileStream(firstDirectoryDB, std::ios::out | std::ios::binary);

    //Creating file operations action file.
    //Contains a list of operations, and paths to do so, of files.
    //Such as "Copy this file here". "delete this file".
    std::wstring fileOpActionFileCreationPath = L"fileOpActionFile.txt";
    std::ofstream fileOpActionFile(fileOpActionFileCreationPath, std::ios::out | std::ios::binary);


    for (int testIter = 0; testIter < directoryDB.size(); ++testIter)
    {
        currentReadLine = directoryDB[testIter]; //Grab item
        writeUnicodeToFile(firstFileStream, currentReadLine); //Write it
    }
    //File operation actions.
    for (int testIter = 0; testIter < fileOpActions.size(); ++testIter)
    {
        currentReadLine = fileOpActions[testIter]; //Grab item
        writeUnicodeToFile(fileOpActionFile, currentReadLine); //Write it
    }


    std::chrono::time_point end = std::chrono::steady_clock::now(); // Stop the clock!
    std::cout << "FINISHED! - " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << std::endl; //Display clock results.


    return 0;
}

//Does as said.
void assignHashTasks(std::vector<std::wstring>& givenDB, std::wstring givenStartPath)
{
    //Pre-calculating size.
    size_t givenDBSize = givenDB.size();
    //Holds output of hashAction reading, allows for manipulation.
    std::wstring currentReadLine;

    //Holds gotten file path.
    std::wstring filePath;

    for (int iterator = 0; iterator < givenDBSize; ++iterator) //Iterating through hashAction file.
    {
        currentReadLine = givenDB[iterator]; //Reading the full line.

        filePath = currentReadLine.substr(0, nthOccurrence(currentReadLine, delimitingCharacter, 1)); //Get file path.

        hashingThreadPool.push_task(MThashGivenFile, filePath, std::ref(givenDB), iterator); //Assigning task.
    }
}

//Converts character arrays to wstring.
std::wstring charToWString(char* givenCharArray)
{
    std::string intermediaryString = givenCharArray;
    int wchars_num = MultiByteToWideChar(65001, 0, intermediaryString.c_str(), -1, NULL, 0);
    wchar_t* wstr = new wchar_t[wchars_num];
    MultiByteToWideChar(65001, 0, intermediaryString.c_str(), -1, wstr, wchars_num);

    return wstr;
}

//Converts char array of MD5 value to a "human readable" hex string.
std::string convertMD5ToHex(unsigned char* givenDigest)
{
    //Create and zero out buffer to hold hex output. - 32 characters given.
    char hexBuffer[32];
    memset(hexBuffer, 0, 32);

    //Creating string to hold final hash output.
    std::string outputHexString;

    //Convert the 128-bit hash to hex.
    for (int i = 0; i < MD5_DIGEST_LENGTH; i++)
    {
        sprintf(hexBuffer, "%02x", givenDigest[i]);
        outputHexString.append(hexBuffer);
    }

    return outputHexString; //Output hash
}


//Creates a list of all files and directories within a given directory. Places each entry in a delimited format into the given wstring vector.
void createDirectoryMapDB(std::vector<std::wstring>& givenVectorDB, std::wstring givenStartPath)
{
    //Making the given path an actual usable path. idk why
    std::filesystem::path dirPath(givenStartPath);
    std::filesystem::directory_iterator end_itr;


    //Creating a stringstream to hold the file size
    std::stringstream temporaryStringStreamFileAndDirSearch;

    std::wstring current_file;
    std::wstringstream testStream;
    //Checking whether to search recursively or not
    if (recursiveSearch) {
        //RECURSIVE
        for (std::filesystem::recursive_directory_iterator end, dir(givenStartPath); dir != end; ++dir)
        {
            current_file = std::filesystem::absolute(dir->path().native());

            //Putting path into array.
            testStream << current_file << delimitingCharacter;

            //Append to DB.
            givenVectorDB.push_back(testStream.str());

            //Clearing stringstream for next iteration.
            testStream.str(std::wstring());
            //}
        }
    }
    else
    {
        // cycle through the directory
        for (std::filesystem::directory_iterator itr(givenStartPath); itr != end_itr; ++itr)
        {
            // If it's not a directory, list it. If you want to list directories too, just remove this check.
            if (std::filesystem::is_regular_file(itr->path()))
            {
                //Setting current file equal to the full path of the file.
                current_file = std::filesystem::absolute(itr->path().native());

                //Putting path into array.
                testStream << current_file << delimitingCharacter + newLine;

                //Append to DB.
                givenVectorDB.push_back(testStream.str());

                //Clearing stringstream for next iteration.
                testStream.str(std::wstring());
            }

        }
    }

}

//Takes a string and removes "\\" and places "/".
std::wstring formatFilePath(std::wstring givenString)
{
    //Formating givenFile to have the slashes ALL be / instead of mixed with \.
    for (int i = 0; i < (int)givenString.length(); ++i)
    {
        if (givenString[i] == '\\')
            givenString[i] = '/';
    }
    return givenString;
}

//Multithreadable hash given file that also handles writing the result to the givenVector.
//Asks for the path to the file to hash, a vector to store output in, and line location in that vector to modify.
void MThashGivenFile(std::wstring givenFilePath, std::vector<std::wstring>& givenVector, std::wstring lineLocation)
{
    //https://www.quora.com/How-can-I-get-the-MD5-or-SHA-hash-of-a-file-in-C
    //Define relevant variables.
    unsigned char digest[MD5_DIGEST_LENGTH];
    char BUFFER[hashBufferSize];
    //Open file to hash for reading.
    std::ifstream fileHandle(givenFilePath, std::ios::binary);

    //Create MD5 handle
    MD5_CTX md5Context;

    //Init the MD5 process.
    MD5_Init(&md5Context);
    //Read until the file is ended.
    while (!fileHandle.eof())
    {
        //Read. Read again. Read.
        fileHandle.read(BUFFER, sizeof(BUFFER));
        //Update the MD5 process. gcount is vital here.
        MD5_Update(&md5Context, BUFFER, fileHandle.gcount());
    }
    //Finish the MD5, place it in the digest.
    int MD5Result = MD5_Final(digest, &md5Context);

    //Verify hash completed successfully.
    if (MD5Result == 0) // Hash failed.
        std::wcout << L"HASH FAILED. 0 RETURNED." + givenFilePath << std::endl;

    //Close file.
    fileHandle.close();
    
    //Write the result to the vector.
    givenVector[_wtoi(lineLocation.c_str())].insert(nthOccurrence(givenVector[_wtoi(lineLocation.c_str())], delimitingCharacter, 1) + 1, stringToWString(convertMD5ToHex(digest)));
}

//returns the position of the nth occurance of a given character. - This could easily be made a string.
//Asks for a string to be searched, character to find, and what count you desire.
size_t nthOccurrence(std::wstring& givenString, std::wstring delimitingCharacter, size_t nth)
{
    size_t stringPosition = 0;
    size_t count = 0;

    while (count != nth)
    {
        stringPosition += 1;
        stringPosition = givenString.find(delimitingCharacter, stringPosition);
        if (stringPosition == std::wstring::npos)
            return -1;

        count++; //Iterate count.
    }
    return stringPosition;
}

void compareHashes(std::vector<std::wstring>& givenDB)
{
    //Pre-calculating vector size.
    size_t givenDBSize = givenDB.size();

    size_t lineLocation;

    std::wstring workingHash;

    std::pair <std::multimap<std::wstring, size_t>::iterator, std::multimap<std::wstring, size_t>::iterator> resultRange; //Holds range to iterate through for matches.

    //Multi map will hold the hash and location.
    std::multimap<std::wstring, size_t> DBMap;

    //Iterate through givenDB and populate DBMap.
    for (size_t iterator = 0; iterator < givenDBSize; ++iterator)
        DBMap.insert(std::make_pair(givenDB[iterator].substr(nthOccurrence(givenDB[iterator], delimitingCharacter, 1), std::wstring::npos - newLine.length()), iterator));

    //Iterate through givenDB. Get Hash value. Look for it in DBMap.
    for (size_t iterator = 0; iterator < givenDBSize; ++iterator)
    {
        //Get hash value.
        workingHash = givenDB[iterator].substr(nthOccurrence(givenDB[iterator], delimitingCharacter, 1), std::wstring::npos - newLine.length());

        //Look for hash in DBMap.
        if (DBMap.count(workingHash) > 1) //Multiple values found associated to this key (the hash).
        {

            //Write initial value
            threadPool.push_task(writeToOutput, L"DUPLICATE" + delimitingCharacter);

            //Create iterator.
            resultRange = DBMap.equal_range(workingHash);

            //Iterate through values.
            for (std::multimap<std::wstring, size_t>::iterator mmIterator = resultRange.first; mmIterator != resultRange.second; ++mmIterator)
            {
                //Write paths.
                threadPool.push_task(writeToOutput, delimitingCharacter + resultRange.second);
            }

            //threadPool.push_task(writeToOutput, givenDB[iterator] + );

            //Write new line.

        }
    }




}


//Performs "filesystem::remove_all" on given path.
void removeObject(std::wstring destinationFilePath, bool recursiveRemoval)
{
    if (!std::filesystem::exists(destinationFilePath)) //Verify it is a normal file. I don't know what other types there are, but I'll avoid deleting them until I know.
        return;

    std::error_code ec; //Create error handler.

    //if (std::filesystem::is_directory(destinationFilePath))
    //    return; //do nothing

    if (recursiveRemoval) //Determine which remove method we are using.
        std::filesystem::remove_all(destinationFilePath, ec); //Removing all.
    else
        std::filesystem::remove(destinationFilePath, ec); //Removing.

    //Sending errors to that error_code seems to fix some problems?
    //An error was occuring sometimes when deleting destination empty directories, but adding this error part make it just work.
    if (ec.value() == 5) //If error value is 5, it is access denied.
        threadPool.push_task(writeToOutput, L"ERROR. ACCESS DENIED: " + destinationFilePath); //Log.

}

//Allows you to multi-thread the process of doign a simple sort of a vector.
void sortDirectoryDatabases(std::vector<std::wstring>& givenVectorDB)
{
    std::sort(givenVectorDB.begin(), givenVectorDB.end());
}

//Converts string to WString.
std::wstring stringToWString(const std::string& s)
{
    std::wstring temp(s.length(), L' ');
    std::copy(s.begin(), s.end(), temp.begin());
    return temp;
}

//Writes to the the debug file.
void writeToOutput(std::wstring textToWrite)
{
    //open the current debug file.
    std::ifstream temporaryDebugHandle(outputFilePath + outputFileName, std::ios::in | std::ios::binary | std::ios::ate); //Start the cursor at the very end.
    size_t fileSize = temporaryDebugHandle.tellg(); //Get the file size in bytes.
    temporaryDebugHandle.close(); //Close temporary handle reading 

    if (fileSize >= 104857600) //100 MB
    {
        //Change the file name.
        if (outputFileName.find(L"."))
            outputFileName = outputFileName.insert(outputFileName.find(L"."), L" - " + std::to_wstring(outputFileCount));
        else
            outputFileName = outputFileName.append(L" - " + std::to_wstring(outputFileCount));
        ++outputFileCount; //Incrememnt debug file count.
    }

    outputFile.open(outputFilePath + outputFileName, std::ios::out | std::ios::binary | std::ios::app); //Open the debug file for writing.

    writeUnicodeToFile(outputFile, textToWrite + newLine); //Write this to the debug file.

    outputFile.close(); //Close file.
}

//Writing unicode to file. Stolen from https://cppcodetips.wordpress.com/2014/09/16/reading-and-writing-a-unicode-file-in-c/
void writeUnicodeToFile(std::ofstream& outputFile, std::wstring inputWstring)
{
    outputFile.write((char*)inputWstring.c_str(), inputWstring.length() * sizeof(wchar_t));
}