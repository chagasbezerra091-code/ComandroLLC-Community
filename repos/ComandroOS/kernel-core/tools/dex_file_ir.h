#ifndef COMANDRO_KERNEL_TOOLS_DEX_FILE_IR_H
#define COMANDRO_KERNEL_TOOLS_DEX_FILE_IR_H

#include <comandro/kernel/types.h>
#include <string>
#include <vector>
#include <map>

namespace comandro {
namespace kernel {
namespace tools {

// =====================================================================
// DEX_FILE_IR_H - Representacao Intermediaria de Arquivos DEX
// Usado para analise e inspecao de baixo nivel.
// =====================================================================

// Estruturas simplificadas para o IR (Intermediate Representation)
struct DexString {
    uint32_t offset;
    std::string value;
};

struct DexType {
    uint32_t string_idx; // Indice na tabela de strings
};

struct DexMethod {
    uint32_t class_idx;  // Indice da classe proprietaria
    uint32_t name_idx;   // Indice do nome do metodo
    uint32_t code_off;   // Offset para o bytecode
};

struct DexIR {
    std::string filename;
    uint32_t checksum;
    uint32_t file_size;

    std::vector<DexString> strings;
    std::vector<DexType> types;
    std::vector<DexMethod> methods;
};

/**
 * @brief O DexFileInspector e o componente de kernel que carrega
 * e analisa arquivos DEX em uma estrutura de IR.
 */
class DexFileInspector {
public:
    /**
     * @brief Carrega um arquivo DEX e preenche a estrutura de IR.
     * @param dex_path O caminho absoluto para o arquivo DEX.
     * @param out_ir A estrutura DexIR a ser preenchida.
     * @return true se o arquivo foi carregado e inspecionado com sucesso.
     */
    bool loadAndInspect(const std::string& dex_path, DexIR& out_ir);

    /**
     * @brief Imprime um resumo detalhado da estrutura DexIR para o console do kernel (usando printf).
     */
    void printIRSummary(const DexIR& ir) const;

private:
    // Funcoes internas para parsing
    bool parseHeader(const uint8_t* base, DexIR& ir);
    bool parseStrings(const uint8_t* base, DexIR& ir);
    bool parseTypes(const uint8_t* base, DexIR& ir);
    bool parseMethods(const uint8_t* base, DexIR& ir);
};

} // namespace tools
} // namespace kernel
} // namespace comandro

#endif // COMANDRO_KERNEL_TOOLS_DEX_FILE_IR_H
