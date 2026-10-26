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

std::unordered_set<std::string> stopWords;

struct DateArticol {
    std::string numeFisier;
    std::string folderDestinatie;
    std::string titluFiltrat;
    std::string textFiltrat;
};

void incarcaStopWords(const std::string& caleFisier) {
    std::ifstream file(caleFisier);
    if (!file.is_open()) return;
    std::string cuvant;
    while (file >> cuvant) {
        std::transform(cuvant.begin(), cuvant.end(), cuvant.begin(), ::tolower);
        stopWords.insert(cuvant);
    }
    file.close();
}

std::string eliminaTaguriXML(const std::string& text) {
    std::string rezultat = "";
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

std::string curataSiFiltreazaText(const std::string& text) {
    std::string faraTaguri = eliminaTaguriXML(text);
    std::string textCurat = "";

    for (char ch : faraTaguri) {
        if (std::isalnum(static_cast<unsigned char>(ch)) || std::isspace(static_cast<unsigned char>(ch))) {
            textCurat += static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        }
        else {
            textCurat += ' ';
        }
    }

    std::stringstream ss(textCurat);
    std::string cuvant;
    std::string rezultat = "";

    while (ss >> cuvant) {
        if ((stopWords.empty() || stopWords.find(cuvant) == stopWords.end()) && cuvant.length() > 1) {
            rezultat += cuvant + " ";
        }
    }

    return rezultat;
}

std::string extrageEticheta(const std::string& continut, const std::string& eticheta) {
    std::string continutLower = continut;
    std::transform(continutLower.begin(), continutLower.end(), continutLower.begin(), ::tolower);

    std::string tagStart = "<" + eticheta;
    std::string tagEnd = "</" + eticheta + ">";
    std::transform(tagStart.begin(), tagStart.end(), tagStart.begin(), ::tolower);
    std::transform(tagEnd.begin(), tagEnd.end(), tagEnd.begin(), ::tolower);

    size_t posStart = continutLower.find(tagStart);
    if (posStart == std::string::npos) return "";

    size_t posStartClose = continutLower.find(">", posStart);
    if (posStartClose == std::string::npos) return "";

    size_t posEnd = continutLower.find(tagEnd, posStartClose);
    if (posEnd == std::string::npos || posEnd <= posStartClose) return "";

    size_t dataStart = posStartClose + 1;
    size_t dataLen = posEnd - dataStart;

    return continut.substr(dataStart, dataLen);
}

std::string getNumeFisierFaraExtensie(const std::string& numeFisier) {
    size_t lastDot = numeFisier.find_last_of(".");
    if (lastDot == std::string::npos) return numeFisier;
    return numeFisier.substr(0, lastDot);
}

void colecteazaArticole(const std::string& folderSursa, const std::string& folderDestinatie, std::vector<DateArticol>& listaArticole) {
    std::string cautarePath = folderSursa + "\\*";
    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA(cautarePath.c_str(), &findData);

    if (hFind == INVALID_HANDLE_VALUE) return;

    do {
        std::string numeElement = findData.cFileName;
        if (numeElement == "." || numeElement == "..") continue;

        std::string caleCompleta = folderSursa + "\\" + numeElement;

        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            colecteazaArticole(caleCompleta, folderDestinatie, listaArticole);
        }
        else {
            std::string numeLower = numeElement;
            std::transform(numeLower.begin(), numeLower.end(), numeLower.begin(), ::tolower);

            if (numeLower.size() >= 4 && numeLower.substr(numeLower.size() - 4) == ".xml") {
                std::ifstream fileIn(caleCompleta);
                if (!fileIn.is_open()) continue;

                std::string continut((std::istreambuf_iterator<char>(fileIn)), std::istreambuf_iterator<char>());
                fileIn.close();

                std::string titluBrut = extrageEticheta(continut, "title");
                std::string textBrut = extrageEticheta(continut, "text");
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

void curataFisiereVechiLaComun(const std::string& folderRezultate) {
    std::string cautarePath = folderRezultate + "\\*.txt";
    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA(cautarePath.c_str(), &findData);

    if (hFind == INVALID_HANDLE_VALUE) return;

    do {
        std::string numeElement = findData.cFileName;
        std::string caleFisier = folderRezultate + "\\" + numeElement;
        DeleteFileA(caleFisier.c_str());
    } while (FindNextFileA(hFind, &findData));

    FindClose(hFind);
}

int main() {
    std::string radacinaProiect = "C:\\Users\\Alex\\Documents\\alexandra_anIII\\sisteme_de_recunoastere\\Reuters\\Reuters_34";

    std::string srcTesting = radacinaProiect + "\\Testing";
    std::string srcTraining = radacinaProiect + "\\Training";
    std::string destTesting = radacinaProiect + "\\Rezultate_TXT\\Testing";
    std::string destTraining = radacinaProiect + "\\Rezultate_TXT\\Training";

    CreateDirectoryA((radacinaProiect + "\\Rezultate_TXT").c_str(), NULL);
    CreateDirectoryA(destTesting.c_str(), NULL);
    CreateDirectoryA(destTraining.c_str(), NULL);

    curataFisiereVechiLaComun(radacinaProiect + "\\Rezultate_TXT");
    incarcaStopWords(radacinaProiect + "\\stopwords.txt");

    std::vector<DateArticol> toateArticolele;

    std::cout << "\nColectare si filtrare articole...\n";
    colecteazaArticole(srcTesting, destTesting, toateArticolele);
    colecteazaArticole(srcTraining, destTraining, toateArticolele);
    std::cout << "Total articole colectate: " << toateArticolele.size() << "\n\n";

    // ETAPA 1: Construirea Dicționarului Global
    std::unordered_map<std::string, int> dictionarGlobal;
    int indexCurent = 0;

    for (const auto& art : toateArticolele) {
        std::stringstream ss(art.titluFiltrat + " " + art.textFiltrat);
        std::string cuvant;
        while (ss >> cuvant) {
            if (dictionarGlobal.find(cuvant) == dictionarGlobal.end()) {
                dictionarGlobal[cuvant] = indexCurent++;
            }
        }
    }

    std::cout << "[OK] Dictionar global construit. Total cuvinte unice: " << dictionarGlobal.size() << "\n";

    // Salvăm dicționarul global sortat după index
    std::vector<std::pair<int, std::string>> dictionarSortat;
    for (auto const& [cuvant, idx] : dictionarGlobal) {
        dictionarSortat.push_back({ idx, cuvant });
    }
    std::sort(dictionarSortat.begin(), dictionarSortat.end());

    std::ofstream fDict(radacinaProiect + "\\Rezultate_TXT\\dictionar_global.txt");
    for (auto const& item : dictionarSortat) {
        fDict << item.first << " : " << item.second << "\n";
    }
    fDict.close();

    // ETAPA 2: Generarea liniilor cu vectorul de apariții pentru fiecare fisier
    std::cout << "Generare fisier vectori de aparitii...\n";
    for (const auto& art : toateArticolele) {
        std::unordered_map<int, int> frecventeArticol;

        std::stringstream ss(art.titluFiltrat + " " + art.textFiltrat);
        std::string cuvant;
        while (ss >> cuvant) {
            int idx = dictionarGlobal[cuvant];
            frecventeArticol[idx]++;
        }

        std::vector<std::pair<int, int>> vectorAparitii(frecventeArticol.begin(), frecventeArticol.end());
        std::sort(vectorAparitii.begin(), vectorAparitii.end());

        std::string caleTxt = art.folderDestinatie + "\\" + getNumeFisierFaraExtensie(art.numeFisier) + ".txt";
        std::ofstream fileOut(caleTxt);

        if (fileOut.is_open()) {
            // Formatul cerut: V nume_fisier.xml: 0:2, 1:1, 2:11
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

    std::cout << "[OK] Procesare finalizata cu succes!\n";

    return 0;
}