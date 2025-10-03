#ifndef __PROJECT1_CPP__
#define __PROJECT1_CPP__

#include "project1.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <iostream>
#include <sstream>
#include <fstream>

int main(int argc, char* argv[]) {
    if (argc < 4) // Checks that at least 3 arguments are given in command line
    {
        std::cerr << "Expected Usage:\n ./assemble infile1.asm infile2.asm ... infilek.asm staticmem_outfile.bin instructions_outfile.bin\n" << std::endl;
        exit(1);
    }
    //Prepare output files
    std::ofstream inst_outfile, static_outfile;
    static_outfile.open(argv[argc - 2], std::ios::binary);
    inst_outfile.open(argv[argc - 1], std::ios::binary);
    std::vector<std::string> instructions; // instructions vector for phase 3
    struct StaticLine{
        std::string label;
        std::vector<std::string> values;
    }; // to store unprocessed instruction labels
    std::vector<StaticLine> static_lines; // static memory variables

    /**
     * Phase 1:
     * Read all instructions, clean them of comments and whitespace DONE
     * TODO: Determine the numbers for all static memory labels
     * (measured in bytes starting at 0)
     * TODO: Determine the line numbers of all instruction line labels
     * (measured in instructions) starting at 0
    */

    int static_addr = 0; // beginning of static addr
    int instr_addr = 0; // beginning of instr addr

    //For each input file:
    for (int i = 1; i < argc - 2; i++) {
        std::ifstream infile(argv[i]); //  open the input file for reading
        if (!infile) { // if file can't be opened, need to let the user know
            std::cerr << "Error: could not open file: " << argv[i] << std::endl;
            exit(1);
        }

        std::string str;
        bool inData = false; 
        bool inText = false;
        while (getline(infile, str)){ //Read a line from the file
            str = clean(str); // remove comments, leading and trailing whitespace
            if (str == "") { //Ignore empty lines
                continue;
            }
            if (str == ".data"){
                inData = true;
                inText = false;
                continue;
            }
            if (str == ".text" || str == ".main"){ // if there is no .text, find .main
                inData = false;
                inText = true;
                continue;
            }

            if (inData){ // we are in static memory area
                // label: .word ...
                auto parts = split(str, WHITESPACE);
                if (parts.size() >= 2 && parts[1] == ".word"){
                    std::string label = parts[0];
                    if(label.back() == ':') label.pop_back();
                    statics[label] = static_addr;
                    std::vector<std::string> vals(parts.begin()+2, parts.end());
                    static_lines.push_back({label, vals});
                    static_addr += 4*vals.size(); // increment address for each word
                }
            }
            else if (inText){ // we are in instructions area
                if (str.back() == ':'){
                    std::string label = str.substr(0, str.size()-1);
                    labels[label] = instr_addr*4;
                }
                else{
                    instructions.push_back(str);
                    instr_addr++;
                }
            }
        }
        infile.close();
    }

    /** Phase 2
     * Process all static memory, output to static memory file
     * TODO: All of this
     */

    for (auto& line : static_lines) {
        for (auto& token : line.values) {
            int val = 0;
            // check if token is numeric
            try {
                val = std::stoi(token);
            } catch (std::invalid_argument&) {
                // token must be a label
                if (statics.find(token) != statics.end()){
                    val = statics[token];
                }
                else if (labels.find(token) != labels.end()){
                    val = labels[token]-4;
                }
            }
            write_binary(val, static_outfile);
        }
    }
    
    
    /** Phase 3
     * Process all instructions, output to instruction memory file
     * TODO: Almost all of this, it only works for adds
     */
    int pc = 0;
    for(std::string inst : instructions) {
        std::vector<std::string> terms = split(inst, WHITESPACE+",()");
        std::string inst_type = terms[0];
        if (inst_type == "add") {
            int result = encode_Rtype(0,registers[terms[2]], registers[terms[3]], registers[terms[1]], 0, 32);
            write_binary(encode_Rtype(0,registers[terms[2]], registers[terms[3]], registers[terms[1]], 0, 32),inst_outfile);
        }
        else if(inst_type == "addi") {
            write_binary(encode_Itype(8, registers[terms[2]], registers[terms[1]], std::stoi(terms[3])), inst_outfile);
        }
        else if(inst_type == "sub"){
            write_binary(encode_Rtype(0, registers[terms[2]], registers[terms[3]], registers[terms[1]], 0, 34),inst_outfile);
        }
        else if(inst_type == "mult"){
            write_binary(encode_Rtype(0, registers[terms[1]], registers[terms[2]], 0, 0, 24), inst_outfile);
        }
        else if(inst_type == "div"){
            write_binary(encode_Rtype(0, registers[terms[1]], registers[terms[2]], 0, 0, 26), inst_outfile);
        }
        else if(inst_type == "mflo"){
            write_binary(encode_Rtype(0, 0, 0, registers[terms[1]], 0, 18), inst_outfile);
        }
        else if(inst_type == "mfhi"){
            write_binary(encode_Rtype(0, 0, 0, registers[terms[1]], 0, 16), inst_outfile);
        }
        else if (inst_type == "la") {
            std::string label = terms[2];              // la $reg, label
            int address = statics[label];              // address of label
            int rd = registers[terms[1]];
            write_binary(encode_Itype(0x08, 0, rd, address), inst_outfile);
        }
        else if(inst_type == "sll"){       
            write_binary(encode_Rtype(0, 0, registers[terms[2]], registers[terms[1]], std::stoi(terms[3]), 0), inst_outfile);
        }
        else if(inst_type == "srl"){
            write_binary(encode_Rtype(0, 0, registers[terms[2]], registers[terms[1]], std::stoi(terms[3]), 2), inst_outfile);
        }
        else if(inst_type == "lw"){
            int offset = stoi(terms[2]);
            write_binary(encode_Itype(0x23, registers[terms[3]], registers[terms[1]], offset), inst_outfile);
        }
        else if(inst_type == "sw") {
            int offset = std::stoi(terms[2]);
            write_binary(encode_Itype(0x2B, registers[terms[3]], registers[terms[1]], offset), inst_outfile);
        }
        else if(inst_type == "slt") {
            write_binary(encode_Rtype(0, registers[terms[2]], registers[terms[3]], registers[terms[1]], 0, 42), inst_outfile);
        }
        else if(inst_type == "beq") {
            int target = labels[terms[3]]/4; 
            int pc_offset = (target - pc - 1); // relative branch
            write_binary(encode_Itype(0x04, registers[terms[1]], registers[terms[2]], pc_offset), inst_outfile);
        }
        else if(inst_type == "bne") {
            int target = labels[terms[3]]/4; 
            int pc_offset = (target - pc - 1);
            write_binary(encode_Itype(0x05, registers[terms[1]], registers[terms[2]], pc_offset), inst_outfile);
        }
        else if(inst_type == "j") {
            int addr = labels[terms[1]]/4-1;
            write_binary(encode_Jtype(0x02, addr), inst_outfile);
        }
        else if(inst_type == "jal") {
            int addr = labels[terms[1]]/4-1;
            write_binary(encode_Jtype(0x03, addr), inst_outfile);
        }
        else if(inst_type == "jr") {
            write_binary(encode_Rtype(0, registers[terms[1]], 0, 0, 0, 8), inst_outfile);
        }
        else if(inst_type == "jalr") {
            write_binary(encode_Rtype(0, registers[terms[1]], 0, 0, 0, 9), inst_outfile);
        }
        else if(inst_type == "syscall") {
            write_binary(encode_Rtype(0, 0, 0, 26, 0, 12), inst_outfile);
        }
        pc++;
    }
}

#endif