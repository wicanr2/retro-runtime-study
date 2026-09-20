/* 預設串流與 stdio 的關係：cout/cerr 到主控台、cin 的 EOF、
   isatty(1)、printf 與 cout 各自緩衝、sync_with_stdio 之後的行為。
   可比對的事實寫進 STD.TXT；主控台輸出順序的觀察記在 README。 */
#include <iostream.h>
#include <fstream.h>
#include <io.h>
#include <stdio.h>

int main()
{
    ofstream f("STD.TXT");
    if (!f)
        return 1;

    f << "isatty1=" << isatty(1) << "\n";

    int x = -7;
    cin >> x;
    f << "cin-empty: fail=" << (cin.fail() ? 1 : 0)
      << " eof=" << (cin.eof() ? 1 : 0)
      << " x-untouched=" << (x == -7 ? 1 : 0) << "\n";
    cin.clear();

    // cout/cerr/clog 都綁好了：能寫、狀態正常
    f << "cout-state: good=" << (cout.good() ? 1 : 0)
      << " cerr-unitbuf="
      << ((cerr.flags() & ios::unitbuf) ? 1 : 0)
      << " cout-unitbuf=" << ((cout.flags() & ios::unitbuf) ? 1 : 0)
      << " cout-tied-from-cin=" << ((cin.tie() == &cout) ? 1 : 0) << "\n";

    printf("printf-line\n");
    cout << "cout-line\n";
    ios::sync_with_stdio();
    printf("after-sync-printf\n");
    cout << "after-sync-cout\n";

    f << "sync-done=1\n";
    return 0;
}
