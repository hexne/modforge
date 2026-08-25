import std;
import modforge.table;

int test_table() {
    modforge::Table table(2, 2);
    table.set_title({"A", "B"});
    table.set_row(1, {"x", "y"});
    table.set_text_pos(0, 1, modforge::TextPos::right);
    table.set_text(1, 0, "value");

    if (table.row() != 2) return 1;
    if (table.col() != 2) return 2;
    if (table[0, 0].text != "A") return 3;
    if (table[1, 1].text != "y") return 4;

    return 0;
}
