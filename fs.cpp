#include <iostream>
#include <vector>
#include <map>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

using namespace std;
#define DISK_SIZE 256
#define DISK_SIM_FILE "DISK_SIM_FILE.txt"

char decToBinary(int n, char & c) {
    // array to store binary number
    int binaryNum[8];

    // counter for binary array
    int i = 0;
    while (n > 0) {
        // storing remainder in binary array
        binaryNum[i] = n % 2;
        n = n / 2;
        i++;
    }

    // printing binary array in reverse order
    for (int j = i - 1; j >= 0; j--) {
        if (binaryNum[j] == 1)
            c = c | 1u << j;
    }
    return '\0';
}
class FsFile {
    int file_size;
    int block_in_use;
    int index_block;
    int block_size;

    int howManyBitsInBlock;

public:
    FsFile(int _block_size) {
        file_size = 0;
        block_in_use = 0;
        block_size = _block_size;
        index_block = -1;
        howManyBitsInBlock = 0;
    }

    int getfile_size() {
        return file_size;
    }
    int getBlockInUse() {
        return block_in_use;
    }
    int getIndexBlock() {
        return index_block;
    }
    int GetBlockSize() {
        return block_size;
    }
    void plusBlockInUse() {
        block_in_use++;
    }
    void setIndexBlock(int index) {
        index_block = index;
    }
    void SetFileSize(int newSize) {
        file_size = newSize;
    }
    void setHowManyBitsInBlock(int bit) {
        howManyBitsInBlock += bit;
    }

};

// ============================================================================

class FileDescriptor {
    string file_name;
    FsFile * fs_file;
    bool inUse;
public:
    FileDescriptor(string FileName, FsFile * fsi) {
        file_name = FileName;
        fs_file = fsi;
        inUse = true;
    }
    string getFileName() {
        return file_name;
    }
    FsFile * getFsi() {
        return fs_file;
    }
    bool isInUse() {
        return (inUse);
    }
    void setInUse(bool _inUse) {
        inUse = _inUse;
    }
    void deleteFile() {
        inUse = false;
        free(fs_file);
    }
};
// ============================================================================

class fsDisk {
    FILE * sim_disk_fd;
    bool is_formated;
    // BitVector - "bit" (int) vector, indicate which block in the disk is free
    //              or not.  (i.e. if BitVector[0] == 1 , means that the
    //             first block is occupied.
    int BitVectorSize;
    int * BitVector;
    // filename and one fsFile.
    // MainDir;
    map < string, FsFile * > MainDir;
    // OpenFileDescriptors --  when you open a file,
    // the operating system creates an entry to represent that file
    // This entry number is the file descriptor.
    vector < FileDescriptor > OpenFileDescriptors;
    // OpenFileDescriptors;
    int direct_enteris;
    int block_size;
    int maxSize;
    int freeBlocks;
private:
    int numBlocksInUse = 0;
    int numDescriptor = 0;
    int NumEmptyBlocks() {
        int counter = 0;
        for (int j = 0; j < BitVectorSize; j++) {
            if (BitVector[j] == 0) {
                counter++;
            }
        }
        return counter;
    }
    // ------------------------------------------------------------------------

public:
    fsDisk() {
        sim_disk_fd = fopen(DISK_SIM_FILE, "r+");
        assert(sim_disk_fd);
        for (int i = 0; i < DISK_SIZE; i++) {
            int ret_val = fseek(sim_disk_fd, i, SEEK_SET);
            ret_val = fwrite("\0", 1, 1, sim_disk_fd);
            assert(ret_val == 1);
        }
        fflush(sim_disk_fd);
        direct_enteris = 0;
        block_size = 0;
        is_formated = false;
    }

    // ------------------------------------------------------------------------
    void listAll() {
        int i = 0;
        for (auto it = begin(OpenFileDescriptors); it != end(OpenFileDescriptors); ++it) {
            cout << "index: " << i << ": FileName: " << it -> getFileName() << " , isInUse: " << it -> isInUse() << endl;
            i++;
        }
        char bufy;
        cout << "Disk content: '";
        for (i = 0; i < DISK_SIZE; i++) {
            int ret_val = fseek(sim_disk_fd, i, SEEK_SET);
            ret_val = fread( & bufy, 1, 1, sim_disk_fd);
            cout << bufy;
        }
        cout << "'" << endl;
    }
    // ------------------------------------------------------------------------
    void fsFormat(int blockSize) {
        if (is_formated == true) { //The file has already been formatted
            return;
        }
        block_size = blockSize;
        direct_enteris = direct_enteris;
        BitVectorSize = DISK_SIZE / block_size;
        BitVector = (int * ) malloc(BitVectorSize * sizeof(int));
        assert(BitVector != NULL);
        for (int j = 0; j < BitVectorSize; j++) {
            BitVector[j] = 0;
        }
        is_formated = true;
    }
    // ------------------------------------------------------------------------
    int CreateFile(string fileName) {
        if (is_formated == false) // The disk was not formatted
            return -1;

        if (MainDir.find(fileName) != MainDir.end()) // The file is already in the system
            return -1;
        if (numBlocksInUse == BitVectorSize) //There are no free blocks on disk
            return -1;
        else {
            FsFile * tmp = new FsFile(block_size);
            OpenFileDescriptors.push_back(FileDescriptor(fileName, tmp));
            MainDir.insert(pair < string, FsFile * > (fileName, tmp));
            numDescriptor++;
            return (numDescriptor - 1);
        }
    }

    // ------------------------------------------------------------------------
    int OpenFile(string fileName) {

        if (is_formated == false) { //The disk hasn't been formatted
            return -1;
        }
        for (int j = 0; j < OpenFileDescriptors.size(); j++) {
            if (fileName.compare(OpenFileDescriptors[j].getFileName()) == 0) {
                if (OpenFileDescriptors[j].isInUse()) { // The file is already open
                    return j;
                } else {
                    OpenFileDescriptors[j].setInUse(true);
                    return j;
                }
            }
        }
        return -1; // The file dosen't exist
    }

    // ------------------------------------------------------------------------
    string CloseFile(int fd) {
        if (is_formated == false) //The disk hasn't been formatted
            return "-1";

        if ((fd < 0) || (fd > OpenFileDescriptors.size())) { //Invalid index
            return "-1";
        }
        if (OpenFileDescriptors[fd].isInUse() == false) { // The file is closed
            return "-1";
        }
        OpenFileDescriptors[fd].setInUse(false);
        return "1";
    }
    // ------------------------------------------------------------------------
    int WriteToFile(int fd, char * buf, int len) {

        if (is_formated == false) { //The disk hasn't been formatted
            return -1;
        }
        if ((fd < 0) || (fd > OpenFileDescriptors.size() - 1)) { //There is no file
            return -1;
        }
        FileDescriptor file = OpenFileDescriptors[fd];
        int len2 = 0; //Length of what we wrote
        if (file.isInUse() == false) { // The file is closed
            return -1;
        }
        if (len > (((file.getFsi() -> getBlockInUse() + block_size) * block_size) - file.getFsi() -> getfile_size())) //  No space in the file
            return -1;

        if (len > (((file.getFsi() -> getBlockInUse() + block_size) * block_size) - file.getFsi() -> getfile_size()))
            len = (((file.getFsi() -> getBlockInUse() + block_size) * block_size) - file.getFsi() -> getfile_size());
        int offSet = file.getFsi() -> getfile_size() % block_size;
        if (len > (NumEmptyBlocks() * block_size + (block_size - offSet))) //No space in the disk
            {
            return -1;
            }
        int ret_val;
        int p = file.getFsi() -> getIndexBlock();
        int blocksInUse = file.getFsi() -> getBlockInUse();
        if (offSet > 0) // Inside an open block
            {
            int index = (file.getFsi() -> getIndexBlock() * block_size) + blocksInUse - 2;
            char bufy = '\0';
            ret_val = fseek(sim_disk_fd, index, SEEK_SET);
            ret_val = fread( & bufy, 1, 1, sim_disk_fd);
            assert(ret_val == 1);
            ret_val = fseek(sim_disk_fd, (int) bufy * block_size + offSet, SEEK_SET);
            for (int i = block_size - offSet; i > 0 && len2 < len; i--) //Stop internal fragmentation
                {
                ret_val = fseek(sim_disk_fd, ((int) bufy * block_size) + offSet + len2, SEEK_SET);
                ret_val = fwrite( & buf[len2], 1, 1, sim_disk_fd);
                assert(ret_val == 1);
                len2++;
                }
            fflush(sim_disk_fd);
            }
        for (int k = 0;
        (k < BitVectorSize) && (len2 < len); k++) {
            if (BitVector[k] == 0) {
                {
                    if (blocksInUse == 0)
                        file.getFsi() -> setIndexBlock(k);
                    else {
                        char temp;
                        char c = '\0';
                        temp = decToBinary(k, c);
                        ret_val = fseek(sim_disk_fd, (file.getFsi() -> getIndexBlock() * block_size) + blocksInUse - 1, SEEK_SET); // ??????
                        ret_val = fwrite( & c, 1, 1, sim_disk_fd);
                        assert(ret_val == 1);
                        ret_val = fseek(sim_disk_fd, k * block_size, SEEK_SET);
                        for (int i = 0; i < block_size && len2 < len; i++) {
                            ret_val = fwrite(buf + len2, 1, 1, sim_disk_fd);
                            assert(ret_val == 1);
                            len2++;
                        }
                    }
                }
                fflush(sim_disk_fd);
                BitVector[k] = 1;
                file.getFsi() -> plusBlockInUse();
                blocksInUse++;
                numBlocksInUse++;
            }
        }
        file.getFsi() -> SetFileSize(file.getFsi() -> getfile_size() + len2);
        cout << "Writed: " << strlen(buf) << " Char's into File Descriptor #: " << fd << endl;
        return 0;
    }
    // ------------------------------------------------------------------------
    int DelFile(string FileName) {

        if (is_formated == false) { // The disk hasn't been formatted
            return -1;
        }
        MainDir.erase(FileName);
        for (int k = 0; k < OpenFileDescriptors.size(); k++) {
            if (FileName.compare(OpenFileDescriptors[k].getFileName()) == 0) {
                if (OpenFileDescriptors[k].isInUse() == true) {
                    // cout << "Cannot delete an open file " << endl;
                    return -1;
                }
                OpenFileDescriptors[k].deleteFile();
                return k;
            }
        }
        return -1; // no file
    }
    // ------------------------------------------------------------------------
    int ReadFromFile(int fd, char * buf, int len) {
        if (is_formated == false) { //The disk hasn't been formatted
            buf[0] = '\0';
            return -1;
        }
        if ((fd < 0) || (fd > OpenFileDescriptors.size() - 1)) { // No such file
            buf[0] = '\0';
            return -1;
        }
        FileDescriptor file = OpenFileDescriptors[fd];
        if (file.isInUse() == false) { //The file is closed
            buf[0] = '\0';
            return -1;
        }
        int index, ret_val;
        int fileSize = file.getFsi() -> getfile_size();
        int len2 = 0;
        char tmp;
        for (int i = 0; i < ((len / block_size) + 1) && len2 < len && len2 < fileSize; i++) {
            int t = file.getFsi() -> getIndexBlock();
            index = (file.getFsi() -> getIndexBlock() * block_size) + i;
            ret_val = fseek(sim_disk_fd, index, SEEK_SET);
            ret_val = fread( & tmp, 1, 1, sim_disk_fd);
            assert(ret_val == 1);
            ret_val = fseek(sim_disk_fd, (int) tmp * block_size, SEEK_SET);
            for (int j = 0;
            (j < block_size) && (len2 < len) && (len2 < fileSize); j++) {
                ret_val = fread( & tmp, 1, 1, sim_disk_fd);
                assert(ret_val == 1);
                buf[len2] = tmp;
                len2++;
            }
        }
        buf[len2] = '\0';
        return len2;
    }
};

int main() {
    int blockSize;
    int direct_entries;
    string fileName;
    char str_to_write[DISK_SIZE];
    char str_to_read[DISK_SIZE];
    int size_to_read;
    int _fd;
    fsDisk * fs = new fsDisk();
    int cmd_;

    while (1) {
        cin >> cmd_;
        switch (cmd_) {
            case 0: // exit
            delete fs;
            exit(0);
            break;

            case 1: // list-file
            fs -> listAll();
            break;

            case 2: // format
            cin >> blockSize;
            fs -> fsFormat(blockSize);
            break;

            case 3: // creat-file
            cin >> fileName;
            _fd = fs -> CreateFile(fileName);
            cout << "CreateFile: " << fileName << " with File Descriptor #: " << _fd << endl;
            break;

            case 4: // open-file
            cin >> fileName;
            _fd = fs -> OpenFile(fileName);
            cout << "OpenFile: " << fileName << " with File Descriptor #: " << _fd << endl;
            break;

            case 5: // close-file
            cin >> _fd;
            fileName = fs -> CloseFile(_fd);
            cout << "CloseFile: " << fileName << " with File Descriptor #: " << _fd << endl;
            break;

            case 6: // write-file
            cin >> _fd;
            cin >> str_to_write;
            fs -> WriteToFile(_fd, str_to_write, strlen(str_to_write));
            break;

            case 7: // read-file
            cin >> _fd;
            cin >> size_to_read;
            fs -> ReadFromFile(_fd, str_to_read, size_to_read);
            cout << "ReadFromFile: " << str_to_read << endl;
            break;

            case 8: // delete file
            cin >> fileName;
            _fd = fs -> DelFile(fileName);
            cout << "DeletedFile: " << fileName << " with File Descriptor #: " << _fd << endl;
            break;
            default:
                break;
        }
    }
}
