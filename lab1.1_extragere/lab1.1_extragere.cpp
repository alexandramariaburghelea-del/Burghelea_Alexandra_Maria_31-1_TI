#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <windows.h>
#include "PorterStemmer.h"

using namespace std;

unordered_set<string> stopWords;
PorterStemmer stemmer;

struct DateArticol {
    string numeFisier;
    string folderDestinatie;
    string clasa;   // categoria extrasa din XML (ex: C15, M11, E21)
    string tipSet;  // "train" sau "test"
    string titluFiltrat;
    string textFiltrat;
};

struct DocumentParsat {
    unordered_map<int, int> frecvente; // index_atribut -> frecventa
    string clasa;
    string tipSet;
    string linieOriginala;
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
            cuvant = stemmer.stemWord(cuvant);
            if (cuvant.length() > 1) {
                rezultat += cuvant + " ";
            }
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

// Extrage clasa (topic-ul) din fisierul XML Reuters
string extrageClasaDinXML(const string& continut) {
    string continutLower = continut;
    transform(continutLower.begin(), continutLower.end(), continutLower.begin(), ::tolower);

    // 1. Format Reuters RCV1: <codes class="bip:topics:1.0"><code code="C15">...
    size_t posTopics = continutLower.find("bip:topics:1.0");
    if (posTopics != string::npos) {
        size_t posEndCodes = continutLower.find("</codes>", posTopics);
        if (posEndCodes == string::npos) posEndCodes = continut.size();

        string primulCod = "";
        size_t searchPos = posTopics;
        while (searchPos < posEndCodes) {
            size_t posCodeAttr = continutLower.find("code=", searchPos);
            if (posCodeAttr == string::npos || posCodeAttr >= posEndCodes) break;

            size_t quoteStart = posCodeAttr + 5;
            if (quoteStart < posEndCodes && (continut[quoteStart] == '"' || continut[quoteStart] == '\'')) {
                char quoteChar = continut[quoteStart];
                size_t quoteEnd = continut.find(quoteChar, quoteStart + 1);
                if (quoteEnd != string::npos && quoteEnd < posEndCodes) {
                    string cod = continut.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
                    if (primulCod.empty()) primulCod = cod;
                    // Preferam codul specific (care nu se termina in "CAT", gen C15, M11, E21)
                    if (cod.size() < 3 || cod.substr(cod.size() - 3) != "CAT") {
                        return cod;
                    }
                    searchPos = quoteEnd + 1;
                    continue;
                }
            }
            searchPos = posCodeAttr + 5;
        }
        if (!primulCod.empty()) return primulCod;
    }

    // 2. Format alternativ Reuters: <topics><d>earn</d></topics>
    string tagD = extrageEticheta(continut, "d");
    if (!tagD.empty()) return tagD;

    return "necunoscut";
}

string getNumeFisierFaraExtensie(const string& numeFisier) {
    size_t lastDot = numeFisier.find_last_of(".");
    if (lastDot == string::npos) return numeFisier;
    return numeFisier.substr(0, lastDot);
}

void colecteazaArticole(const string& folderSursa, const string& folderDestinatie,
    const string& tipSet, const string& clasaCurenta,
    vector<DateArticol>& listaArticole) {
    string cautarePath = folderSursa + "\\*";
    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA(cautarePath.c_str(), &findData);

    if (hFind == INVALID_HANDLE_VALUE) return;

    do {
        string numeElement = findData.cFileName;
        if (numeElement == "." || numeElement == "..") continue;

        string caleCompleta = folderSursa + "\\" + numeElement;

        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            colecteazaArticole(caleCompleta, folderDestinatie, tipSet, numeElement, listaArticole);
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
                art.clasa = !clasaCurenta.empty() ? clasaCurenta : extrageClasaDinXML(continut);
                art.tipSet = tipSet;
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

double calculeazaEntropie(const unordered_map<string, int>& numarPerClasa, int totalDocumente) {
    if (totalDocumente == 0) return 0.0;
    double entropie = 0.0;
    for (const auto& [clasa, count] : numarPerClasa) {
        if (count > 0) {
            double p = static_cast<double>(count) / totalDocumente;
            entropie -= p * log2(p);
        }
    }
    return entropie;
}

void executaEtapa1_CalculIG(const string& fisierIntrareTxt, const string& fisierIesireTxt) {
    ifstream fIn(fisierIntrareTxt);
    if (!fIn.is_open()) {
        cerr << "Eroare la deschiderea fisierului: " << fisierIntrareTxt << "\n";
        return;
    }

    vector<string> numeAtribute;
    vector<DocumentParsat> documente;
    unordered_map<string, int> totalPerClasa;

    string linie;
    bool inData = false;

    while (getline(fIn, linie)) {
        if (linie.empty()) continue;

        if (linie.rfind("@attribute ", 0) == 0) {
            string nume = linie.substr(11);
            while (!nume.empty() && isspace(static_cast<unsigned char>(nume.back()))) {
                nume.pop_back();
            }
            numeAtribute.push_back(nume);
        }
        else if (linie == "@data") {
            inData = true;
        }
        else if (inData) {
            size_t posPrimDiez = linie.find('#');
            if (posPrimDiez == string::npos) continue;
            size_t posAlDoileaDiez = linie.find('#', posPrimDiez + 1);
            if (posAlDoileaDiez == string::npos) continue;

            DocumentParsat doc;
            doc.linieOriginala = linie;

            string parteVector = linie.substr(0, posPrimDiez);
            string clasa = linie.substr(posPrimDiez + 1, posAlDoileaDiez - posPrimDiez - 1);
            string tipSet = linie.substr(posAlDoileaDiez + 1);

            auto trim = [](string& s) {
                while (!s.empty() && isspace(static_cast<unsigned char>(s.front()))) s.erase(s.begin());
                while (!s.empty() && isspace(static_cast<unsigned char>(s.back()))) s.pop_back();
                };
            trim(clasa);
            trim(tipSet);

            doc.clasa = clasa;
            doc.tipSet = tipSet;
            totalPerClasa[clasa]++;

            stringstream ss(parteVector);
            string pereche;
            while (ss >> pereche) {
                size_t douaPuncte = pereche.find(':');
                if (douaPuncte != string::npos) {
                    int idx = stoi(pereche.substr(0, douaPuncte));
                    int val = stoi(pereche.substr(douaPuncte + 1));
                    doc.frecvente[idx] = val;
                }
            }
            documente.push_back(doc);
        }
    }
    fIn.close();

    int totalDocs = static_cast<int>(documente.size());
    double entropieTotala = calculeazaEntropie(totalPerClasa, totalDocs);

    cout << "Clase detectate (" << totalPerClasa.size() << "): ";
    for (const auto& [c, cnt] : totalPerClasa) {
        cout << c << "(" << cnt << ") ";
    }
    cout << "\nEntropia generala Entropy(S) = " << entropieTotala << "\n";

    int nrAtribute = static_cast<int>(numeAtribute.size());
    vector<int> docsCuAtribut(nrAtribute + 1, 0);
    vector<unordered_map<string, int>> claseCuAtribut(nrAtribute + 1);

    for (const auto& doc : documente) {
        for (const auto& [idxAtrib, val] : doc.frecvente) {
            if (val > 0 && idxAtrib <= nrAtribute) {
                docsCuAtribut[idxAtrib]++;
                claseCuAtribut[idxAtrib][doc.clasa]++;
            }
        }
    }

    ofstream fOut(fisierIesireTxt);
    for (int idx = 1; idx <= nrAtribute; ++idx) {
        int countPrezent = docsCuAtribut[idx];
        int countAbsent = totalDocs - countPrezent;

        unordered_map<string, int> claseFaraAtribut;
        for (const auto& [clasa, totalClasa] : totalPerClasa) {
            int inPrezent = claseCuAtribut[idx][clasa];
            claseFaraAtribut[clasa] = totalClasa - inPrezent;
        }

        double entropiePrezent = calculeazaEntropie(claseCuAtribut[idx], countPrezent);
        double entropieAbsent = calculeazaEntropie(claseFaraAtribut, countAbsent);

        double pPrezent = static_cast<double>(countPrezent) / totalDocs;
        double pAbsent = static_cast<double>(countAbsent) / totalDocs;

        double ig = entropieTotala - (pPrezent * entropiePrezent) - (pAbsent * entropieAbsent);
        if (ig < 1e-12) ig = 0.0;

        fOut << "@attribute " << numeAtribute[idx - 1] << " " << ig << "\n";
    }

    fOut << "@data\n";
    for (const auto& doc : documente) {
        fOut << doc.linieOriginala << "\n";
    }
    fOut.close();

    cout << "[OK] Etapa 1 finalizata! Rezultatul a fost salvat in: " << fisierIesireTxt << "\n";
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

    colecteazaArticole(srcTesting, destTesting, "test", "", toateArticolele);
    colecteazaArticole(srcTraining, destTraining, "train", "", toateArticolele);
    cout << "Total articole colectate: " << toateArticolele.size() << "\n\n";

    unordered_map<string, int> dictionarGlobal;
    int indexCurent = 1;

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

    string caleFisierLab2 = radacinaProiect + "\\Rezultate_TXT\\fisier_lab2_complet.txt";
    ofstream fLab2(caleFisierLab2);

    for (auto const& [idx, cuvant] : dictionarSortat) {
        fLab2 << "@attribute " << cuvant << "\n";
    }
    fLab2 << "@data\n";

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
            fileOut << "TITLU: " << art.titluFiltrat << "\n";
            fileOut << "TEXT: " << art.textFiltrat << "\n\n";

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

        for (size_t i = 0; i < vectorAparitii.size(); ++i) {
            fLab2 << vectorAparitii[i].first << ":" << vectorAparitii[i].second << " ";
        }
        fLab2 << "# " << art.clasa << " # " << art.tipSet << "\n";
    }

    fLab2.close();

    string caleEtapa1 = radacinaProiect + "\\Rezultate_TXT\\etapa1_ig.txt";
    executaEtapa1_CalculIG(caleFisierLab2, caleEtapa1);

    return 0;
}