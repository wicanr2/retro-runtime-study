/* 全域物件的建構與解構順序：Iostream_init（優先序 16）先於使用者全域建構（32），
   所以全域物件的建構式裡 cout 已經可以用；解構在結束鏈反向執行。
   輸出 CTOR.TXT；另在主控台印一行證明「建構式裡 cout 已可用」。 */
#include <iostream.h>
#include <fstream.h>

ofstream log("CTOR.TXT");

class Marker
{
public:
    const char *s;
    Marker(const char *p) : s(p)
    {
        cout << "[ctor " << s << "]\n";   // cout 在 Iostream_init 之後才可用
        log << "ctor:" << s << "\n";
    }
    ~Marker()
    {
        log << "dtor:" << s << "\n";
    }
};

Marker m1("global-1");
Marker m2("global-2");

int main()
{
    log << "main\n";
    return 0;
}
