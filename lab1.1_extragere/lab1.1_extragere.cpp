#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_set>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <windows.h>

std::unordered_set<std::string> stopWords;

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

void proceseazaFisier(const std::string& caleXml, const std::string& folderDestinatie, const std::string& numeFisier) {
    std::ifstream fileIn(caleXml);
    if (!fileIn.is_open()) return;

    std::string continut((std::istreambuf_iterator<char>(fileIn)), std::istreambuf_iterator<char>());
    fileIn.close();

    std::string titluBrut = extrageEticheta(continut, "title");
    std::string textBrut = extrageEticheta(continut, "text");
    if (textBrut.empty()) {
        textBrut = extrageEticheta(continut, "body");
    }

    std::string titluFiltrat = curataSiFiltreazaText(titluBrut);
    std::string textFiltrat = curataSiFiltreazaText(textBrut);

    std::string caleTxt = folderDestinatie + "\\" + getNumeFisierFaraExtensie(numeFisier) + ".txt";
    std::ofstream fileOut(caleTxt);

    if (fileOut.is_open()) {
        fileOut << "TITLU:\n" << titluFiltrat << "\n\n";
        fileOut << "TEXT:\n" << textFiltrat << "\n";
        fileOut.close();
        std::cout << "OK " << numeFisier << " -> " << caleTxt << std::endl;
    }
}

void cautaSiProceseaza(const std::string& folderSursa, const std::string& folderDestinatie, int& contor) {
    std::string cautarePath = folderSursa + "\\*";
    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA(cautarePath.c_str(), &findData);

    if (hFind == INVALID_HANDLE_VALUE) return;

    do {
        std::string numeElement = findData.cFileName;

        if (numeElement == "." || numeElement == "..") continue;

        std::string caleCompleta = folderSursa + "\\" + numeElement;

        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            cautaSiProceseaza(caleCompleta, folderDestinatie, contor);
        }
        else {
            std::string numeLower = numeElement;
            std::transform(numeLower.begin(), numeLower.end(), numeLower.begin(), ::tolower);

            if (numeLower.size() >= 4 && numeLower.substr(numeLower.size() - 4) == ".xml") {
                proceseazaFisier(caleCompleta, folderDestinatie, numeElement);
                contor++;
            }
        }
    } while (FindNextFileA(hFind, &findData));

    FindClose(hFind);
}


int main() {
    std::string radacinaProiect = "C:\\Users\\Student\\source\\repos\\lab1.1_extragere\\lab1.1_extragere";

    std::string srcTesting = radacinaProiect + "\\Testing";
    std::string srcTraining = radacinaProiect + "\\Training";
    std::string destTesting = radacinaProiect + "\\Rezultate_TXT\\Testing";
    std::string destTraining = radacinaProiect + "\\Rezultate_TXT\\Training";

    CreateDirectoryA((radacinaProiect + "\\Rezultate_TXT").c_str(), NULL);
    CreateDirectoryA(destTesting.c_str(), NULL);
    CreateDirectoryA(destTraining.c_str(), NULL);

    incarcaStopWords(radacinaProiect + "\\stopwords.txt");

    int contorTest = 0;
    int contorTrain = 0;

    cautaSiProceseaza(srcTesting, destTesting, contorTest);

    cautaSiProceseaza(srcTraining, destTraining, contorTrain);
    std::cout << " - Fisiere din Testing procesate: " << contorTest << "\n";
    std::cout << " - Fisiere din Training procesate: " << contorTrain << "\n\n";
    std::cout <<destTesting << "\n";
    std::cout << destTraining << std::endl;

    return 0;
}