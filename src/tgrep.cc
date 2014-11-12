#include <vector>
#include <set>
#include <string>
#include <sstream>
#include <sys/types.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <algorithm>
#include <string.h>



namespace IO {
    void xaddSig(int sig, int id) {
	static int l_sig = -1;
	static int fd = -1;

	if (l_sig != sig) {
	    l_sig = sig;
	    if (fd != -1) close(fd);
	    std::stringstream name;
	    name << "/var/tgrep/" << sig;       
	    fd = open(name.str().c_str(),O_CREAT+O_APPEND+O_RDWR, 0666);
	    if (fd < 0) {
		perror(name.str().c_str());
		exit(-1);
	    }
	}
	write(fd,&id, sizeof(id));
    }

    void addSig(int sig, int id) {
	std::stringstream name;
	name << "/var/tgrep/spool";       
	static int fd = open(name.str().c_str(),O_CREAT+O_APPEND+O_RDWR, 0666);
	if (fd < 0) {
	    perror(name.str().c_str());
	    exit(-1);
	}
	char buf[100];
	sprintf(buf,"%d %d\n",sig,id);
	write(fd,buf,strlen(buf));
	//close(fd);
    }


    std::string getSig(int sig) {
	std::stringstream name;
	name << "/var/tgrep/" << sig;
	return name.str();
    }


    int addFile(const std::string&fname) {
	std::stringstream name;
	name << "/var/tgrep/names";       
	static int fd = open(name.str().c_str(),O_CREAT+O_APPEND+O_RDWR, 0666);
	if (fd < 0) {
	    perror(name.str().c_str());
	    exit(-1);
	}
	int id = lseek(fd,0, SEEK_END);
	short len = fname.size();
	write(fd,&len,sizeof(len));
	write(fd,fname.c_str(), fname.size()+1);
	//close(fd);
	return id;
    }
    
    std::string getFile(int id) {
	std::stringstream name;
	name << "/var/tgrep/names";       
	int fd = open(name.str().c_str(),O_CREAT+O_APPEND+O_RDWR, 0666);
	if (fd < 0) {
	    perror(name.str().c_str());
	    exit(-1);
	}
	lseek(fd,id, SEEK_SET);
	short len;
	read(fd,&len,sizeof(len));
	char buf[len+10];
	read(fd,buf, len+1);
	close(fd);

	//std::cerr << id << std::endl;
	//std::cerr << len << std::endl;
	//std::cerr << buf << std::endl;

	return std::string(buf);
    }

};

class Map
{
    
    void *m_addr;
    unsigned long m_size;
public:
    Map(std::string name) {
	int fd = open(name.c_str(),O_RDWR);
	if (fd < 0) {
	    m_addr = 0;
	    m_size = 0;
	    return;
	}
	m_size = lseek(fd, 0, SEEK_END);
	if (m_size == 0) {
	    m_addr = 0;
	    m_size = 0;
	    return;
	}
	lseek(fd, 0, SEEK_SET);
	m_addr = mmap(0, m_size, PROT_READ+PROT_WRITE,MAP_PRIVATE,fd,0);

	close(fd);
    }
    ~Map() {
	if (m_addr)
	    munmap(m_addr, m_size);
    }
	
    int  *c_int() { return (int*)m_addr; }
    char *c_str() { return (char*)m_addr; }
    unsigned long size() { return m_size; }
    unsigned long sizeInt() { return m_size/sizeof(int); }

};




class Filename 
{

public:
    typedef long Fileid;

    Filename() = delete;
    Filename(const std::string& name) { m_id = IO::addFile(name); }
    Filename(Fileid id):m_id(id) {};
    
    std::string getName() { return IO::getFile(m_id); }
    Fileid getId() { return m_id; }

private:
    Fileid m_id;
};


class Filenames 
{
    std::vector<Filename> m_names;
public:
    Filenames();    
};
    
class Index 
{
    bool m_first;
    std::vector<int> m_res;

public:
    Index():m_first(true) {}
    
    void addSig(Filename& fn, int sig) {
	IO::addSig(sig, fn.getId());
    }
    
    void merge(int sig)	{
	Map sigs(IO::getSig(sig));
	int* pint = sigs.c_int();
	int len = sigs.sizeInt();

	if (pint == 0) {
	    m_res.clear();
	    m_first = false;
	    return;
	}

	std::sort(&pint[0], &pint[len]);
	
	//std::cerr << "SIG "<<sig << std::endl;
	//for (int x=0; x<len; x++) 
	//    std::cerr << '['<<pint[x]<<"] ";
	//std::cerr << std::endl;
	

	if (m_first) {
	    m_first = false;
	    for (int i=0; i <len; i++) m_res.push_back(pint[i]);
	} else {
	    std::vector<int> m_intersect;
	    auto a = m_res.begin();
	    int  b = 0;
	    while (b<len && a != m_res.end()) {
		if (*a == pint[b]) {
		    m_intersect.push_back(pint[b]);
		    b++;
		    a++;
		} else
		if (*a < pint[b]) 
		    a++;
		else
		    b++;
	    }
	    m_res = m_intersect;
	}	
    }

    std::vector<int> & getRes() { return m_res; }
};



class TextQuads
{
    std::set<int> m_quad;
    void addText(const char*txt, unsigned int len)
	{
	    if (len>=4)
		for (unsigned i=0; i <len-3; i++) {
		    // we make the shifts hetrogenous to create variations between shifts while keeping the keyspace low(ish).
		    int val = (txt[i+0] << 17) + (txt[i+1] << 12) + (txt[i+2] << 8) + (txt[i+3]);
		    m_quad.insert(val);
		}
	}
public:
    TextQuads(){}
    TextQuads(const char*txt, int len) { addText(txt,len); }
    TextQuads(std::string& text)       { addText(text.c_str(),text.size()); }

    const std::set<int>& getSignature() { return m_quad; } 
};
    

    
    
    
int main(int argc, char**argv)
{
    if (argc > 1 && strcmp(argv[1],"--index") == 0) {
	for (int f=2; f < argc; f++) {
	    
	    Filename filename(argv[f]);
	    Map filetext(argv[f]);
	    TextQuads quads(filetext.c_str(), filetext.size());
	    Index index;
	    	    
	    for (auto i : quads.getSignature()) {
		index.addSig(filename,i);
	    }
	}
    } else 
    if (argc > 1 && strcmp(argv[1],"--unspool") == 0) {
	FILE *spool = fopen("/var/tgrep/spool","r");
	int sig, id;
	while (fscanf(spool,"%d %d\n",&sig,&id) == 2) {
	    IO::xaddSig(sig,id);
	}
	fclose(spool);
    } else
    if ( argc >=2) {
	Index index;

	for (int p=1; p < argc; p++) {
	    TextQuads quads(argv[p], strlen(argv[p]));
      	
	    for (auto i : quads.getSignature()) {
		index.merge(i);
	    }
	}

	for (auto i : index.getRes())
	{

	    Filename name(i);
	    std::cout <<  name.getName() << std::endl;
	}
	
    }    
}
