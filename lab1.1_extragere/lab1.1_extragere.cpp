#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <windows.h>

using namespace std;


unordered_set<string> stopWords;

struct DateArticol {
    string numeFisier;
    string folderDestinatie;
    string titluFiltrat;
    string textFiltrat;
};

void incarcaStopWords(const string& caleFisier) {
    ifstream file(caleFisier);

    if (!file.is_open()) return;
    string cuvant;
    while (file >> cuvant) {
        transform(cuvant.begin(), cuvant.end(), cuvant.begin(), ::tolower);
        stopWords.insert(cuvant);
    }
    file.close();
}

string eliminaTaguriXML(const string& text) {
    string rezultat = "";
    bool inTag = false;
    for (char ch : text) {
        if (ch == '<') {
            inTag = true;
        }
        else if (ch == '>') {
            inTag = false;
            rezultat += ' ';
        }
        else if (!inTag) {
            rezultat += ch;
        }
    }
    return rezultat;
}

string curataSiFiltreazaText(const string& text) {
    string faraTaguri = eliminaTaguriXML(text);
    string textCurat = "";

    for (char ch : faraTaguri) {
        if (isalnum(static_cast<unsigned char>(ch)) || isspace(static_cast<unsigned char>(ch))) {
            textCurat += static_cast<char>(tolower(static_cast<unsigned char>(ch)));
        }
        else {
            textCurat += ' ';
        }
    }

    stringstream ss(textCurat);
    string cuvant;
    string rezultat = "";

    while (ss >> cuvant) {
        if ((stopWords.empty() || stopWords.find(cuvant) == stopWords.end()) && cuvant.length() > 1) {
            rezultat += cuvant + " ";
        }
    }

    return rezultat;
}

string extrageEticheta(const string& continut, const string& eticheta) {
    string continutLower = continut;
    transform(continutLower.begin(), continutLower.end(), continutLower.begin(), ::tolower);

    string tagStart = "<" + eticheta;
    string tagEnd = "</" + eticheta + ">";
    transform(tagStart.begin(), tagStart.end(), tagStart.begin(), ::tolower);
    transform(tagEnd.begin(), tagEnd.end(), tagEnd.begin(), ::tolower);

    size_t posStart = continutLower.find(tagStart);
    if (posStart == string::npos) return "";

    size_t posStartClose = continutLower.find(">", posStart);
    if (posStartClose == string::npos) return "";

    size_t posEnd = continutLower.find(tagEnd, posStartClose);
    if (posEnd == string::npos || posEnd <= posStartClose) return "";

    size_t dataStart = posStartClose + 1;
    size_t dataLen = posEnd - dataStart;

    return continut.substr(dataStart, dataLen);
}

string getNumeFisierFaraExtensie(const string& numeFisier) {
    size_t lastDot = numeFisier.find_last_of(".");
    if (lastDot == string::npos) return numeFisier;
    return numeFisier.substr(0, lastDot);
}


void colecteazaArticole(const string& folderSursa, const string& folderDestinatie, vector<DateArticol>& listaArticole) {
    string cautarePath = folderSursa + "\\*";

    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA(cautarePath.c_str(), &findData);

    if (hFind == INVALID_HANDLE_VALUE) return;

    do {

        string numeElement = findData.cFileName;

        if (numeElement == "." || numeElement == "..") continue;

        string caleCompleta = folderSursa + "\\" + numeElement;

        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            colecteazaArticole(caleCompleta, folderDestinatie, listaArticole);
        }
        else {
            string numeLower = numeElement;
            transform(numeLower.begin(), numeLower.end(), numeLower.begin(), ::tolower);

            if (numeLower.size() >= 4 && numeLower.substr(numeLower.size() - 4) == ".xml") {

                ifstream fileIn(caleCompleta);
                if (!fileIn.is_open()) continue;

                string continut((istreambuf_iterator<char>(fileIn)), istreambuf_iterator<char>());
                fileIn.close();

                string titluBrut = extrageEticheta(continut, "title");
                string textBrut = extrageEticheta(continut, "text");

                if (textBrut.empty()) {
                    textBrut = extrageEticheta(continut, "body");
                }

                DateArticol art;
                art.numeFisier = numeElement;
                art.folderDestinatie = folderDestinatie;
                art.titluFiltrat = curataSiFiltreazaText(titluBrut);
                art.textFiltrat = curataSiFiltreazaText(textBrut);

                listaArticole.push_back(art);
            }
        }
    } while (FindNextFileA(hFind, &findData));

    FindClose(hFind);
}
void curataFisiereVechiLaComun(const string& folderRezultate) {
    string cautarePath = folderRezultate + "\\*.txt";

    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA(cautarePath.c_str(), &findData);

    if (hFind == INVALID_HANDLE_VALUE) return;

    do {

        string numeElement = findData.cFileName;
        string caleFisier = folderRezultate + "\\" + numeElement;

        DeleteFileA(caleFisier.c_str());
    } while (FindNextFileA(hFind, &findData));

    FindClose(hFind);
}

int main() {

    string radacinaProiect = "C:\\Users\\Alex\\Documents\\alexandra_anIII\\sisteme_de_recunoastere\\Reuters\\Reuters_34";


    string srcTesting = radacinaProiect + "\\Testing";
    string srcTraining = radacinaProiect + "\\Training";
    string destTesting = radacinaProiect + "\\Rezultate_TXT\\Testing";
    string destTraining = radacinaProiect + "\\Rezultate_TXT\\Training";

    CreateDirectoryA((radacinaProiect + "\\Rezultate_TXT").c_str(), NULL);
    CreateDirectoryA(destTesting.c_str(), NULL);
    CreateDirectoryA(destTraining.c_str(), NULL);

    curataFisiereVechiLaComun(radacinaProiect + "\\Rezultate_TXT");
    incarcaStopWords(radacinaProiect + "\\stopwords.txt");


    vector<DateArticol> toateArticolele;

    cout << "\nColectare si filtrare articole...\n";
    colecteazaArticole(srcTesting, destTesting, toateArticolele);
    colecteazaArticole(srcTraining, destTraining, toateArticolele);
    cout << "Total articole colectate: " << toateArticolele.size() << "\n\n";

    unordered_map<string, int> dictionarGlobal;
    int indexCurent = 0;

    for (const auto& art : toateArticolele) {
        stringstream ss(art.titluFiltrat + " " + art.textFiltrat);
        string cuvant;
        while (ss >> cuvant) {
            if (dictionarGlobal.find(cuvant) == dictionarGlobal.end()) {
                dictionarGlobal[cuvant] = indexCurent++;
            }
        }
    }

    cout << "Dictionar global construit. Total cuvinte unice: " << dictionarGlobal.size() << "\n";

    vector<pair<int, string>> dictionarSortat;
    for (auto const& [cuvant, idx] : dictionarGlobal) {
        dictionarSortat.push_back({ idx, cuvant });
    }
    sort(dictionarSortat.begin(), dictionarSortat.end());

    ofstream fDict(radacinaProiect + "\\Rezultate_TXT\\dictionar_global.txt");
    for (auto const& [idx, cuvant] : dictionarSortat) {
        fDict << idx << " : " << cuvant << "\n";
    }
    fDict.close();

    cout << "Generare fisier vectori de aparitii...\n";
    for (const auto& art : toateArticolele) {
        unordered_map<int, int> frecventeArticol;

        stringstream ss(art.titluFiltrat + " " + art.textFiltrat);
        string cuvant;

        while (ss >> cuvant) {
            int idx = dictionarGlobal[cuvant];
            frecventeArticol[idx]++;
        }


        vector<pair<int, int>> vectorAparitii(frecventeArticol.begin(), frecventeArticol.end());
        sort(vectorAparitii.begin(), vectorAparitii.end());

        string caleTxt = art.folderDestinatie + "\\" + getNumeFisierFaraExtensie(art.numeFisier) + ".txt";
        ofstream fileOut(caleTxt);

        if (fileOut.is_open()) {

            fileOut << "V " << art.numeFisier << ": ";
            for (size_t i = 0; i < vectorAparitii.size(); ++i) {
                fileOut << vectorAparitii[i].first << ":" << vectorAparitii[i].second;
                if (i + 1 < vectorAparitii.size()) {
                    fileOut << ", ";
                }
            }
            fileOut << "\n";
            fileOut.close();
        }
    }

    cout << "[OK] Procesare finalizata cu succes!\n";

    return 0;
}