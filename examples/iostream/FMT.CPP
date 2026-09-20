/* iostream 的格式化行為：旗標、width 一次性、fill、internal、浮點預設。
   輸出 FMT.TXT（用 ofstream，順便驗 filebuf 開檔寫入）。 */
#include <iostream.h>
#include <fstream.h>
#include <iomanip.h>

int main()
{
    ofstream f("FMT.TXT");
    if (!f)
        return 1;

    f << "int: " << 42 << " " << -42 << " " << 0 << "\n";
    f << "long: " << 123456789L << "\n";

    f << "hex: " << hex << 255 << " " << 0x1a << "\n";
    f << "oct: " << oct << 255 << "\n";
    f.setf(ios::uppercase);
    f << "hex-upper: " << hex << 255 << "\n";
    f.unsetf(ios::uppercase);
    f.setf(ios::showbase);
    f << "showbase-hex: " << hex << 255 << "\n";
    f << "showbase-oct: " << oct << 255 << "\n";
    f.unsetf(ios::showbase);
    f << "dec-restore: " << dec << 255 << "\n";

    f << "showpos-dec: " << setiosflags(ios::showpos) << 42 << "\n";
    f.unsetf(ios::showpos);

    // width 是一次性的：只影響下一筆
    f.width(6);
    f << 42 << "|" << 42 << "\n";
    f << setw(6) << 42 << "|" << 42 << "\n";

    f.fill('0');
    f.width(6);
    f << 42 << "\n";
    f.fill(' ');

    f << setiosflags(ios::left) << setw(6) << 42 << "|\n";
    f << resetiosflags(ios::adjustfield) << setiosflags(ios::right) << setw(6) << 42 << "|\n";
    f << setiosflags(ios::showpos | ios::internal) << setw(6) << 42 << "|\n";
    f.unsetf(ios::showpos | ios::internal);

    // 浮點：precision 預設 0（路徑上視為 6 位小數）
    f << "flt-default: " << 3.5 << " " << 0.25 << " " << 12345.6789 << "\n";
    f << "flt-prec2: " << setprecision(2) << 3.14159 << " " << 12345.0 << "\n";
    f << "flt-fixed: " << setiosflags(ios::fixed) << 3.14159 << "\n";
    f.unsetf(ios::floatfield);
    f << "flt-sci: " << setiosflags(ios::scientific) << 3.14159 << "\n";
    f.unsetf(ios::floatfield);
    f << "flt-small: " << 0.00001 << " " << 0.0001 << "\n";
    f << "flt-restore: " << 3.14159 << "\n";

    // 字串與字元不受 width 以外的旗標影響
    f << "str: " << "abc" << "\n";
    f << "char: " << 'x' << "\n";
    f << "endl-test" << endl << "after-endl\n";
    return 0;
}
