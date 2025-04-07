#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <string>
#include <sstream>
#include <cctype>
#include <filesystem>
#include <cmath>
#include <stdexcept>

using namespace std;
namespace fs = std::filesystem;

struct Header {
    char idLength;
    char colorMapType;
    char dataTypeCode;
    short colorMapOrigin;
    short colorMapLength;
    char colorMapDepth;
    short xOrigin;
    short yOrigin;
    short width;
    short height;
    char bitsPerPixel;
    char imageDescriptor;
};

struct Pixel {
    unsigned char B;
    unsigned char G;
    unsigned char R;
};

struct Image {
    Header header;
    vector<Pixel> pixels;
};

Image readImage(const string& path) {
    ifstream file(path, ios::binary);
    if (!file.is_open()) {
        cerr << "Failed to open input file: " << path << endl;
        exit(1);
    }

    Header h;
    file.read(&h.idLength, 1);
    file.read(&h.colorMapType, 1);
    file.read(&h.dataTypeCode, 1);
    file.read(reinterpret_cast<char*>(&h.colorMapOrigin), 2);
    file.read(reinterpret_cast<char*>(&h.colorMapLength), 2);
    file.read(&h.colorMapDepth, 1);
    file.read(reinterpret_cast<char*>(&h.xOrigin), 2);
    file.read(reinterpret_cast<char*>(&h.yOrigin), 2);
    file.read(reinterpret_cast<char*>(&h.width), 2);
    file.read(reinterpret_cast<char*>(&h.height), 2);
    file.read(&h.bitsPerPixel, 1);
    file.read(&h.imageDescriptor, 1);

    vector<Pixel> pixels(h.width * h.height);
    for (int i = 0; i < pixels.size(); i++) {
        file.read(reinterpret_cast<char*>(&pixels[i].B), 1);
        file.read(reinterpret_cast<char*>(&pixels[i].G), 1);
        file.read(reinterpret_cast<char*>(&pixels[i].R), 1);
    }

    Image img;
    img.header = h;
    img.pixels = pixels;
    return img;
}

void writeImage(const Image& img, const string& path) {
    ofstream file(path, ios::binary);
    if (!file.is_open()) {
        cerr << "Failed to create output file: " << path << endl;
        exit(1);
    }

    file.write(&img.header.idLength, 1);
    file.write(&img.header.colorMapType, 1);
    file.write(&img.header.dataTypeCode, 1);
    file.write(reinterpret_cast<const char*>(&img.header.colorMapOrigin), 2);
    file.write(reinterpret_cast<const char*>(&img.header.colorMapLength), 2);
    file.write(&img.header.colorMapDepth, 1);
    file.write(reinterpret_cast<const char*>(&img.header.xOrigin), 2);
    file.write(reinterpret_cast<const char*>(&img.header.yOrigin), 2);
    file.write(reinterpret_cast<const char*>(&img.header.width), 2);
    file.write(reinterpret_cast<const char*>(&img.header.height), 2);
    file.write(&img.header.bitsPerPixel, 1);
    file.write(&img.header.imageDescriptor, 1);

    for (int i = 0; i < img.pixels.size(); i++) {
        file.write(reinterpret_cast<const char*>(&img.pixels[i].B), 1);
        file.write(reinterpret_cast<const char*>(&img.pixels[i].G), 1);
        file.write(reinterpret_cast<const char*>(&img.pixels[i].R), 1);
    }
}

Image multiply(Image a, Image b) {
    vector<Pixel> result;
    for (int i = 0; i < a.pixels.size(); i++) {
        Pixel px;
        px.B = static_cast<unsigned char>((a.pixels[i].B / 255.0f) * (b.pixels[i].B / 255.0f) * 255 + 0.5f);
        px.G = static_cast<unsigned char>((a.pixels[i].G / 255.0f) * (b.pixels[i].G / 255.0f) * 255 + 0.5f);
        px.R = static_cast<unsigned char>((a.pixels[i].R / 255.0f) * (b.pixels[i].R / 255.0f) * 255 + 0.5f);
        result.push_back(px);
    }
    
    Image out;
    out.header = a.header;
    out.pixels = result;
    return out;
}

Image subtract(Image base, Image layer) {
    vector<Pixel> result;
    for (int i = 0; i < base.pixels.size(); i++) {
        Pixel px;
        px.B = max(0, base.pixels[i].B - layer.pixels[i].B);
        px.G = max(0, base.pixels[i].G - layer.pixels[i].G);
        px.R = max(0, base.pixels[i].R - layer.pixels[i].R);
        result.push_back(px);
    }
    
    Image out;
    out.header = base.header;
    out.pixels = result;
    return out;
}

Image screen(Image a, Image b) {
    vector<Pixel> result;
    for (int i = 0; i < a.pixels.size(); i++) {
        Pixel px;
        px.B = static_cast<unsigned char>((1 - (1 - a.pixels[i].B/255.0f)*(1 - b.pixels[i].B/255.0f)) * 255 + 0.5f);
        px.G = static_cast<unsigned char>((1 - (1 - a.pixels[i].G/255.0f)*(1 - b.pixels[i].G/255.0f)) * 255 + 0.5f);
        px.R = static_cast<unsigned char>((1 - (1 - a.pixels[i].R/255.0f)*(1 - b.pixels[i].R/255.0f)) * 255 + 0.5f);
        result.push_back(px);
    }
    
    Image out;
    out.header = a.header;
    out.pixels = result;
    return out;
}

Image overlay(Image base, Image layer) {
    vector<Pixel> result;
    for (int i = 0; i < base.pixels.size(); i++) {
        float (*calc)(float, float) = [](float b, float l) {
            if (l <= 0.5) return 2 * b * l;
            else return 1 - 2 * (1 - b) * (1 - l);
        };
        
        Pixel px;
        px.B = static_cast<unsigned char>(calc(base.pixels[i].B/255.0f, layer.pixels[i].B/255.0f) * 255 + 0.5f);
        px.G = static_cast<unsigned char>(calc(base.pixels[i].G/255.0f, layer.pixels[i].G/255.0f) * 255 + 0.5f);
        px.R = static_cast<unsigned char>(calc(base.pixels[i].R/255.0f, layer.pixels[i].R/255.0f) * 255 + 0.5f);
        result.push_back(px);
    }
    
    Image out;
    out.header = base.header;
    out.pixels = result;
    return out;
}

Image flip(Image img) {
    reverse(img.pixels.begin(), img.pixels.end());
    return img;
}

Image onlyRed(Image img) {
    for (int i = 0; i < img.pixels.size(); i++) {
        img.pixels[i].G = img.pixels[i].R;
        img.pixels[i].B = img.pixels[i].R;
    }
    return img;
}

Image onlyGreen(Image img) {
    for (int i = 0; i < img.pixels.size(); i++) {
        img.pixels[i].R = img.pixels[i].G;
        img.pixels[i].B = img.pixels[i].G;
    }
    return img;
}

Image onlyBlue(Image img) {
    for (int i = 0; i < img.pixels.size(); i++) {
        img.pixels[i].R = img.pixels[i].B;
        img.pixels[i].G = img.pixels[i].B;
    }
    return img;
}

Image addRed(Image img, int val) {
    for (int i = 0; i < img.pixels.size(); i++) {
        img.pixels[i].R = min(255, max(0, img.pixels[i].R + val));
    }
    return img;
}

Image addGreen(Image img, int val) {
    for (int i = 0; i < img.pixels.size(); i++) {
        img.pixels[i].G = min(255, max(0, img.pixels[i].G + val));
    }
    return img;
}

Image addBlue(Image img, int val) {
    for (int i = 0; i < img.pixels.size(); i++) {
        img.pixels[i].B = min(255, max(0, img.pixels[i].B + val));
    }
    return img;
}

Image scaleRed(Image img, float val) {
    for (int i = 0; i < img.pixels.size(); i++) {
        img.pixels[i].R = min(255, max(0, static_cast<int>(img.pixels[i].R * val)));
    }
    return img;
}

Image scaleGreen(Image img, float val) {
    for (int i = 0; i < img.pixels.size(); i++) {
        img.pixels[i].G = min(255, max(0, static_cast<int>(img.pixels[i].G * val)));
    }
    return img;
}

Image scaleBlue(Image img, float val) {
    for (int i = 0; i < img.pixels.size(); i++) {
        img.pixels[i].B = min(255, max(0, static_cast<int>(img.pixels[i].B * val)));
    }
    return img;
}

Image combine(Image red, Image green, Image blue) {
    for (int i = 0; i < red.pixels.size(); i++) {
        red.pixels[i].G = green.pixels[i].G;
        red.pixels[i].B = blue.pixels[i].B;
    }
    return red;
}

int main(int argc, char* argv[]) {
    if (argc == 1 || (argc == 2 && string(argv[1]) == "--help")) {
        cout << "Project 2: Image Processing, Fall 2023" << endl;
        cout << "Usage:" << endl;
        cout << "\t./project2.out [output] [firstImage] [mtd] [...]" << endl;
        return 0;
    }

    if (argc < 3) {
        cerr << "Invalid file name." << endl;
        return 1;
    }

    string outputPath = argv[1];
    if (outputPath.length() < 4 || outputPath.substr(outputPath.length() - 4) != ".tga") {
        cerr << "Invalid file name. Output file must have .tga extension." << endl;
        return 1;
    }

    string inputPath = argv[2];
    if (inputPath.length() < 4 || inputPath.substr(inputPath.length() - 4) != ".tga") {
        cerr << "Invalid file name. Input file must have .tga extension." << endl;
        return 1;
    }

    Image trackingImage = readImage(inputPath);

    int i = 3;
    while (i < argc) {
        string mtd = argv[i++];
        
        if (mtd == "multiply") {
            if (i >= argc) {
                cerr << "Missing argument for multiply operation." << endl;
                return 1;
            }
            string otherPath = argv[i++];
            if (otherPath.length() < 4 || otherPath.substr(otherPath.length() - 4) != ".tga") {
                cerr << "Invalid argument, invalid file name for multiply operation." << endl;
                return 1;
            }
            try {
                Image otherImage = readImage(otherPath);
                trackingImage = multiply(trackingImage, otherImage);
            } catch (...) {
                cerr << "Invalid argument, file does not exist: " << otherPath << endl;
                return 1;
            }
        }

        else if (mtd == "subtract") {
            if (i >= argc) {
                cerr << "Missing argument for subtract operation." << endl;
                return 1;
            }
            string otherPath = argv[i++];
            if (otherPath.length() < 4 || otherPath.substr(otherPath.length() - 4) != ".tga") {
                cerr << "Invalid argument, invalid file name for subtract operation." << endl;
                return 1;
            }
            try {
                Image otherImage = readImage(otherPath);
                trackingImage = subtract(trackingImage, otherImage);
            } catch (...) {
                cerr << "Invalid argument, file does not exist: " << otherPath << endl;
                return 1;
            }
        }

        else if (mtd == "screen") {
            if (i >= argc) {
                cerr << "Missing argument for screen operation." << endl;
                return 1;
            }
            string otherPath = argv[i++];
            if (otherPath.length() < 4 || otherPath.substr(otherPath.length() - 4) != ".tga") {
                cerr << "Invalid argument, invalid file name for screen operation." << endl;
                return 1;
            }
            try {
                Image otherImage = readImage(otherPath);
                trackingImage = screen(trackingImage, otherImage);
            } catch (...) {
                cerr << "Invalid argument, file does not exist: " << otherPath << endl;
                return 1;
            }
        }

        else if (mtd == "overlay") {
            if (i >= argc) {
                cerr << "Missing argument for overlay operation." << endl;
                return 1;
            }
            string otherPath = argv[i++];
            if (otherPath.length() < 4 || otherPath.substr(otherPath.length() - 4) != ".tga") {
                cerr << "Invalid argument, invalid file name for overlay operation." << endl;
                return 1;
            }
            try {
                Image otherImage = readImage(otherPath);
                trackingImage = overlay(trackingImage, otherImage);
            } catch (...) {
                cerr << "Invalid argument, file does not exist: " << otherPath << endl;
                return 1;
            }
        }
        
        else if (mtd == "flip") {
            trackingImage = flip(trackingImage);
        }

        else if (mtd == "onlyred") {
            trackingImage = onlyRed(trackingImage);
        }

        else if (mtd == "onlygreen") {
            trackingImage = onlyGreen(trackingImage);
        }

        else if (mtd == "onlyblue") {
            trackingImage = onlyBlue(trackingImage);
        }

        else if (mtd == "addred") {
            if (i >= argc) {
                cerr << "Missing argument for addred operation." << endl;
                return 1;
            }
            string valStr = argv[i++];
            try {
                int val = stoi(valStr);
                trackingImage = addRed(trackingImage, val);
            } catch (const invalid_argument&) {
                cerr << "Invalid argument for addred, expected a number." << endl;
                return 1;
            } catch (const out_of_range&) {
                cerr << "Invalid argument for addred, number out of range." << endl;
                return 1;
            }
        }

        else if (mtd == "addgreen") {
            if (i >= argc) {
                cerr << "Missing argument for addgreen operation." << endl;
                return 1;
            }
            string valStr = argv[i++];
            try {
                int val = stoi(valStr);
                trackingImage = addGreen(trackingImage, val);
            } catch (const invalid_argument&) {
                cerr << "Invalid argument for addgreen, expected a number." << endl;
                return 1;
            } catch (const out_of_range&) {
                cerr << "Invalid argument for addgreen, number out of range." << endl;
                return 1;
            }
        }

        else if (mtd == "addblue") {
            if (i >= argc) {
                cerr << "Missing argument for addblue operation." << endl;
                return 1;
            }
            string valStr = argv[i++];
            try {
                int val = stoi(valStr);
                trackingImage = addBlue(trackingImage, val);
            } catch (const invalid_argument&) {
                cerr << "Invalid argument for addblue, expected a number." << endl;
                return 1;
            } catch (const out_of_range&) {
                cerr << "Invalid argument for addblue, number out of range." << endl;
                return 1;
            }
        }

        else if (mtd == "scalered") {
            if (i >= argc) {
                cerr << "Missing argument for scalered operation." << endl;
                return 1;
            }
            string valStr = argv[i++];
            try {
                float val = stof(valStr);
                trackingImage = scaleRed(trackingImage, val);
            } catch (const invalid_argument&) {
                cerr << "Invalid argument for scalered, expected a number." << endl;
                return 1;
            } catch (const out_of_range&) {
                cerr << "Invalid argument for scalered, number out of range." << endl;
                return 1;
            }
        }

        else if (mtd == "scalegreen") {
            if (i >= argc) {
                cerr << "Missing argument for scalegreen operation." << endl;
                return 1;
            }
            string valStr = argv[i++];
            try {
                float val = stof(valStr);
                trackingImage = scaleGreen(trackingImage, val);
            } catch (const invalid_argument&) {
                cerr << "Invalid argument for scalegreen, expected a number." << endl;
                return 1;
            } catch (const out_of_range&) {
                cerr << "Invalid argument for scalegreen, number out of range." << endl;
                return 1;
            }
        }

        else if (mtd == "scaleblue") {
            if (i >= argc) {
                cerr << "Missing argument for scaleblue operation." << endl;
                return 1;
            }
            string valStr = argv[i++];
            try {
                float val = stof(valStr);
                trackingImage = scaleBlue(trackingImage, val);
            } catch (const invalid_argument&) {
                cerr << "Invalid argument for scaleblue, expected a number." << endl;
                return 1;
            } catch (const out_of_range&) {
                cerr << "Invalid argument for scaleblue, number out of range." << endl;
                return 1;
            }
        }

        else if (mtd == "combine") {
            if (i + 1 >= argc) {
                cerr << "Missing arguments for combine operation. Need green and blue channel images." << endl;
                return 1;
            }
            string greenPath = argv[i++];
            string bluePath = argv[i++];
            if (greenPath.length() < 4 || greenPath.substr(greenPath.length() - 4) != ".tga" ||
                bluePath.length() < 4 || bluePath.substr(bluePath.length() - 4) != ".tga") {
                cerr << "Invalid argument, invalid file name for combine operation." << endl;
                return 1;
            }
            try {
                Image greenImage = readImage(greenPath);
                Image blueImage = readImage(bluePath);
                trackingImage = combine(trackingImage, greenImage, blueImage);
            } catch (...) {
                cerr << "Invalid argument, file does not exist." << endl;
                return 1;
            }
        }
        
        else {
            cerr << "Invalid mtd name: " << mtd << endl;
            return 1;
        }
    }

    writeImage(trackingImage, outputPath);
    return 0;
}