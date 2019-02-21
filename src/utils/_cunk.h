
#ifndef __CUNK_H__
#define __CUNK_H__

/*
#ifndef max
#define max(a,b) ((a) >= (b) ? (a) : (b))
#endif
*/
class CUnk
{
	long m_cRef;
public:
	CUnk():m_cRef(1){};
	virtual ~CUnk(){};

	long AddRef(){
	    long lRef = InterlockedIncrement(&m_cRef); 
            return std::max<long>(m_cRef, 1);
        };

	long Release(){
            long lRef = InterlockedDecrement(&m_cRef); 
            if(lRef==0){
                m_cRef++; 
                delete this; 
                return 0;
            } 
            return std::max<long>(m_cRef, 1);
        };
};

#endif //__CUNK_H__

