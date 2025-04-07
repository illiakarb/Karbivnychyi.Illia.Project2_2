#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <string>
#include <sstream>
#include <cctype>
#include <cmath>
#include <stdexcept>

using namespace std;

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
        px.B = static_cast<unsigned char>((1 - (1 - a.pixels[i].B / 255.0f) * (1 - b.pixels[i].B / 255.0f)) * 255 + 0.5f);
        px.G = static_cast<unsigned char>((1 - (1 - a.pixels[i].G / 255.0f) * (1 - b.pixels[i].G / 255.0f)) * 255 + 0.5f);
        px.R = static_cast<unsigned char>((1 - (1 - a.pixels[i].R / 255.0f) * (1 - b.pixels[i].R / 255.0f)) * 255 + 0.5f);
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
        px.B = static_cast<unsigned char>(calc(base.pixels[i].B / 255.0f, layer.pixels[i].B / 255.0f) * 255 + 0.5f);
        px.G = static_cast<unsigned char>(calc(base.pixels[i].G / 255.0f, layer.pixels[i].G / 255.0f) * 255 + 0.5f);
        px.R = static_cast<unsigned char>(calc(base.pixels[i].R / 255.0f, layer.pixels[i].R / 255.0f) * 255 + 0.5f);
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

int main(int argc, char** argv) {
    if (argc < 4) {
        cerr << "Usage: " << argv[0] << " <input.tga> <output.tga> <operation>" << endl;
        return 1;
    }

    string input = argv[1];
    string output = argv[2];
    string operation = argv[3];

    ifstream ifile(input);
    if (!ifile) {
        cerr << "Input file does not exist." << endl;
        return 1;
    }

    Image img = readImage(input);

    if (operation == "multiply") {
        if (argc < 5) {
            cerr << "Missing second input image for multiply operation" << endl;
            return 1;
        }
        string second_input = argv[4];
        Image second_img = readImage(second_input);
        img = multiply(img, second_img);
    } 
    else if (operation == "subtract") {
        if (argc < 5) {
            cerr << "Missing second input image for subtract operation" << endl;
            return 1;
        }
        string second_input = argv[4];
        Image second_img = readImage(second_input);
        img = subtract(img, second_img);
    }
    else if (operation == "screen") {
        if (argc < 5) {
            cerr << "Missing second input image for screen operation" << endl;
            return 1;
        }
        string second_input = argv[4];
        Image second_img = readImage(second_input);
        img = screen(img, second_img);
    }
    else if (operation == "overlay") {
        if (argc < 5) {
            cerr << "Missing second input image for overlay operation" << endl;
            return 1;
        }
        string second_input = argv[4];
        Image second_img = readImage(second_input);
        img = overlay(img, second_img);
    }
    else if (operation == "flip") {
        img = flip(img);
    }
    else if (operation == "onlyRed") {
        img = onlyRed(img);
    }
    else if (operation == "onlyGreen") {
        img = onlyGreen(img);
    }
    else if (operation == "onlyBlue") {
        img = onlyBlue(img);
    }
    else if (operation == "addRed") {
        if (argc < 5) {
            cerr << "Missing value for addRed operation" << endl;
            return 1;
        }
        int value = stoi(argv[4]);
        img = addRed(img, value);
    }
    else if (operation == "addGreen") {
        if (argc < 5) {
            cerr << "Missing value for addGreen operation" << endl;
            return 1;
        }
        int value = stoi(argv[4]);
        img = addGreen(img, value);
    }
    else if (operation == "addBlue") {
        if (argc < 5) {
            cerr << "Missing value for addBlue operation" << endl;
            return 1;
        }
        int value = stoi(argv[4]);
        img = addBlue(img, value);
    }
    else if (operation == "scaleRed") {
        if (argc < 5) {
            cerr << "Missing scale factor for scaleRed operation" << endl;
            return 1;
        }
        float scale = stof(argv[4]);
        img = scaleRed(img, scale);
    }
    else if (operation == "scaleGreen") {
        if (argc < 5) {
            cerr << "Missing scale factor for scaleGreen operation" << endl;
            return 1;
        }
        float scale = stof(argv[4]);
        img = scaleGreen(img, scale);
    }
    else if (operation == "scaleBlue") {
        if (argc < 5) {
            cerr << "Missing scale factor for scaleBlue operation" << endl;
            return 1;
        }
        float scale = stof(argv[4]);
        img = scaleBlue(img, scale);
    }

    writeImage(img, output);
    return 0;
}
