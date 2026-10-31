#include<iostream>
#include<string>
#include<vector>
using namespace std;

void generatePattern(const string& input)
{
    int n = input.length();
    bool foundPattern = false;
    for(int len=n/2;len>=1;len--)
    {
        string p = input.substr(0,len);
        int k = 0;
        int i = 0;
        while(i+len <= n && input.substr(i,len) == p)
        {
            k++;
            i += len;
        }
        if(k > 1)
        {
            string q = input.substr(i);
            cout<<"Detected pattern: "<<p<<endl;
            cout<<"Detected suffix:  "<<(q.empty() ? "Empty or Epsilon" : q)<<endl<<endl;
            cout<<"Generated CFG:"<<endl;
            cout<<"S -> "<<p<<"S | "<<(q.empty() ? "e" : q)<<endl;
            cout<<"Generalized Language pattern"<<endl;
            cout<<"L = (" <<p<< ")^n "<<q<<", where(n >= 0)"<<endl;
            foundPattern = true;
            break;
        }
    }
    if(!foundPattern)
    {
        cout<<"No reccuring sequential pattern found."<<endl;
        cout<<"Language pattern:"<<endl;
        cout<<"L = { "<<input<<" }"<<endl;
        cout<<"Generated CFG:"<<endl;
        cout<<"S -> "<<input<<endl;
    }
}

int main()
{
    string input;
    cout<<"Enter the string: ";
    cin>>input;
    generatePattern(input);
    return 0;
}
