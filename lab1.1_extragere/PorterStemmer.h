#pragma once
#include <string>

using namespace std;

class PorterStemmer {
private:
    string wordStr;
    int endIndex;
    int stemIndex;

    bool isConsonant(int index) {
        char c = wordStr[index];
        if (c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u') return false;
        return c != 'y' || (index == 0 || !isConsonant(index - 1));
    }

    int measureConsonantSequence() {
        int n = 0;
        int index = 0;
        while (true) {
            if (index > stemIndex) return n;
            if (!isConsonant(index)) break;
            index++;
        }
        index++;
        while (true) {
            while (true) {
                if (index > stemIndex) return n;
                if (isConsonant(index)) break;
                index++;
            }
            index++;
            n++;
            while (true) {
                if (index > stemIndex) return n;
                if (!isConsonant(index)) break;
                index++;
            }
            index++;
        }
    }

    bool vowelInStem() {
        for (int i = 0; i <= stemIndex; i++) {
            if (!isConsonant(i)) return true;
        }
        return false;
    }

    bool isDoubleConsonant(int index) {
        if (index < 1) return false;
        return wordStr[index] == wordStr[index - 1] && isConsonant(index);
    }

    bool isCVC(int index) {
        if (index < 2 || !isConsonant(index) || isConsonant(index - 1) || !isConsonant(index - 2)) return false;
        char c = wordStr[index];
        return c != 'w' && c != 'x' && c != 'y';
    }

    bool endsWith(const string& s) {
        int length = (int)s.length();
        int index = endIndex - length + 1;
        if (index < 0) return false;

        for (int i = 0; i < length; i++) {
            if (wordStr[index + i] != s[i]) return false;
        }
        stemIndex = endIndex - length;
        return true;
    }

    void setEnd(const string& s) {
        int length = (int)s.length();
        int index = stemIndex + 1;
        for (int i = 0; i < length; i++) {
            if (index + i < (int)wordStr.length()) {
                wordStr[index + i] = s[i];
            }
            else {
                wordStr += s[i];
            }
        }
        endIndex = stemIndex + length;
        wordStr.resize(endIndex + 1);
    }

    void replaceEnd(const string& s) {
        if (measureConsonantSequence() > 0) setEnd(s);
    }

    void step1() {
        if (wordStr[endIndex] == 's') {
            if (endsWith("sses")) {
                endIndex -= 2;
            }
            else if (endsWith("ies")) {
                setEnd("i");
            }
            else if (wordStr[endIndex - 1] != 's') {
                endIndex--;
            }
        }
        if (endsWith("eed")) {
            if (measureConsonantSequence() > 0)
                endIndex--;
        }
        else if ((endsWith("ed") || endsWith("ing")) && vowelInStem()) {
            endIndex = stemIndex;
            if (endsWith("at"))
                setEnd("ate");
            else if (endsWith("bl"))
                setEnd("ble");
            else if (endsWith("iz"))
                setEnd("ize");
            else if (isDoubleConsonant(endIndex)) {
                endIndex--;
                char ch = wordStr[endIndex];
                if (ch == 'l' || ch == 's' || ch == 'z')
                    endIndex++;
            }
            else if (measureConsonantSequence() == 1 && isCVC(endIndex)) setEnd("e");
        }
        wordStr.resize(endIndex + 1);
    }

    void step2() {
        if (endsWith("y") && vowelInStem())
            wordStr[endIndex] = 'i';
    }

    void step3() {
        if (endIndex == 0) return;
        switch (wordStr[endIndex - 1]) {
        case 'a':
            if (endsWith("ational")) { replaceEnd("ate"); break; }
            if (endsWith("tional")) { replaceEnd("tion"); }
            break;
        case 'c':
            if (endsWith("enci")) { replaceEnd("ence"); break; }
            if (endsWith("anci")) { replaceEnd("ance"); }
            break;
        case 'e':
            if (endsWith("izer")) { replaceEnd("ize"); }
            break;
        case 'l':
            if (endsWith("bli")) { replaceEnd("ble"); break; }
            if (endsWith("alli")) { replaceEnd("al"); break; }
            if (endsWith("entli")) { replaceEnd("ent"); break; }
            if (endsWith("eli")) { replaceEnd("e"); break; }
            if (endsWith("ousli")) { replaceEnd("ous"); }
            break;
        case 'o':
            if (endsWith("ization")) { replaceEnd("ize"); break; }
            if (endsWith("ation")) { replaceEnd("ate"); break; }
            if (endsWith("ator")) { replaceEnd("ate"); }
            break;
        case 's':
            if (endsWith("alism")) { replaceEnd("al"); break; }
            if (endsWith("iveness")) { replaceEnd("ive"); break; }
            if (endsWith("fulness")) { replaceEnd("ful"); break; }
            if (endsWith("ousness")) { replaceEnd("ous"); }
            break;
        case 't':
            if (endsWith("aliti")) { replaceEnd("al"); break; }
            if (endsWith("iviti")) { replaceEnd("ive"); break; }
            if (endsWith("biliti")) { replaceEnd("ble"); }
            break;
        case 'g':
            if (endsWith("logi")) { replaceEnd("log"); }
            break;
        }
    }

    void step4() {
        switch (wordStr[endIndex]) {
        case 'e':
            if (endsWith("icate")) { replaceEnd("ic"); break; }
            if (endsWith("ative")) { replaceEnd(""); break; }
            if (endsWith("alize")) { replaceEnd("al"); }
            break;
        case 'i':
            if (endsWith("iciti")) { replaceEnd("ic"); }
            break;
        case 'l':
            if (endsWith("ical")) { replaceEnd("ic"); break; }
            if (endsWith("ful")) { replaceEnd(""); }
            break;
        case 's':
            if (endsWith("ness")) { replaceEnd(""); }
            break;
        }
    }

    void step5() {
        if (endIndex == 0) return;
        switch (wordStr[endIndex - 1]) {
        case 'a': if (endsWith("al")) break; return;
        case 'c': if (endsWith("ance") || endsWith("ence")) break; return;
        case 'e': if (endsWith("er")) break; return;
        case 'i': if (endsWith("ic")) break; return;
        case 'l': if (endsWith("able") || endsWith("ible")) break; return;
        case 'n': if (endsWith("ant") || endsWith("ement") || endsWith("ment") || endsWith("ent")) break; return;
        case 'o': if (endsWith("ion") && stemIndex >= 0 && (wordStr[stemIndex] == 's' || wordStr[stemIndex] == 't')) break;
            if (endsWith("ou")) break; return;
        case 's': if (endsWith("ism")) break; return;
        case 't': if (endsWith("ate") || endsWith("iti")) break; return;
        case 'u': if (endsWith("ous")) break; return;
        case 'v': if (endsWith("ive")) break; return;
        case 'z': if (endsWith("ize")) break; return;
        default: return;
        }
        if (measureConsonantSequence() > 1)
            endIndex = stemIndex;
        wordStr.resize(endIndex + 1);
    }

    void step6() {
        stemIndex = endIndex;
        if (wordStr[endIndex] == 'e') {
            int a = measureConsonantSequence();
            if (a > 1 || (a == 1 && !isCVC(endIndex - 1)))
                endIndex--;
        }
        if (wordStr[endIndex] == 'l' && isDoubleConsonant(endIndex) && measureConsonantSequence() > 1)
            endIndex--;
        wordStr.resize(endIndex + 1);
    }

public:
    string stemWord(const string& word) {
        if (word.empty() || word.length() <= 2) return word;

        wordStr = word;
        stemIndex = 0;
        endIndex = (int)wordStr.length() - 1;

        step1();
        step2();
        step3();
        step4();
        step5();
        step6();

        return wordStr;
    }
};