#include <iostream>
#include <cstdio>
#include <cstring>
#include <ctime>
using namespace std;
static char a[100001000];
void *memset_naive(void* s,int c,size_t n)
{
	for(int i=0;i<n;++i) ((char*)s)[i]=(unsigned char)(c);
}
void *memset_op1(void* s,int c,size_t n)
{
	char c1=(unsigned char)(c);
	unsigned int num_to_fill=(c1<<24)+(c1<<16)+(c1<<8)+c1;
	long long i=0;
	for(;i<=n/4;i++) ((int*)s)[i]=num_to_fill;
	i*=4;
	for(;i<n;++i) ((char*)s)[i]=c1;
}
void *memset_op2(void* s,int c,size_t n)
{
	char c1=(unsigned char)(c);
	unsigned long long num_to_fill=((unsigned long long)c1<<56)+((unsigned long long)c1<<48)+((unsigned long long)c1<<40)+((unsigned long long)c1<<32)+(c1<<24)+(c1<<16)+(c1<<8)+c1;
	long long i=0;
	for(;i<=n/8;i++) ((long long*)s)[i]=num_to_fill;
	i*=8;
	for(;i<n;++i) ((char*)s)[i]=c1;
}
bool check(char c)
{
	bool flag=true;
	for(int i=0;i<sizeof(a);++i)
	{
		if(a[i]!=c)
		{
			flag=false;
			break;
		}
	}
	return flag;
}
int main()
{
	clock_t time;
	
	memset_naive(a,'1',sizeof(a));
	time=clock();
	memset_naive(a,'2',sizeof(a));
	cout << "cpu time: " << clock()-time << " ";
	cout << (check('2')?"succeeded!":"failed!") << endl;
	
	memset_op1(a,'1',sizeof(a));
	time=clock();
	memset_op1(a,'3',sizeof(a));
	cout << "cpu time: " << clock()-time << " ";
	cout << (check('3')?"succeeded!":"failed!") << endl;
	
	memset_op2(a,'1',sizeof(a));
	time=clock();
	memset_op2(a,'4',sizeof(a));
	cout << "cpu time: " << clock()-time << " ";
	cout << (check('4')?"succeeded!":"failed!") << endl;
	return 0;
}
