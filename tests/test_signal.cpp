import std;
import modforge.signal;

int test_signal() {
    modforge::Signal s;

    int a = 0;
    int b = 0;

    s.connect([&]{ ++a; });
    s.connect([&]{ ++b; });

    s.emit();
    if (a != 1) return 1;
    if (b != 1) return 2;

    // connect_once
    modforge::Signal s_once;
    int c = 0;
    s_once.connect_once([&]{ ++c; });
    s_once.emit();
    s_once.emit();
    if (c != 1) return 3;

    // connect_n
    modforge::Signal s_n;
    int d = 0;
    s_n.connect_n([&]{ ++d; }, 3);
    s_n.emit();
    s_n.emit();
    s_n.emit();
    s_n.emit();
    if (d != 3) return 4;

    // multiple connections and persistence
    modforge::Signal s_mix;
    int e = 0;
    s_mix.connect([&]{ ++e; });
    s_mix.connect_once([&]{ e += 10; });
    s_mix.emit(); // e = 1 + 10 = 11
    if (e != 11) return 5;
    s_mix.emit(); // e += 1 => 12
    if (e != 12) return 6;

    return 0;
}
