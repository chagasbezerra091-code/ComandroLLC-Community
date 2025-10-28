#include "dex_file_ir.h"
#include <comandro/kernel/log.h>
#include <comandro/kernel/fs/file_io.h>
#include <cstdio> // Inclui a biblioteca C para printf
#include <cstring>

namespace comandro {
namespace kernel {
namespace tools {

using kernel::Log;
using kernel::fs::FileIO;

static constexpr const char* TAG = "DexInspector";
static constexpr uint32_t DEX_MAGIC_SIZE = 8;
static constexpr uint32_t DEX_HEADER_SIZE = 112;

// Assumindo a estrutura de dados bruta de um arquivo DEX (simplificada)
struct DexHeaderRaw {
    uint8_t magic[DEX_MAGIC_SIZE];
    uint32_t checksum;
    uint32_t file_size;
    // ... outros campos (links, mapa, strings, types, fields, methods, classes)
    uint32_t string_ids_size;
    uint32_t string_ids_off;
    uint32_t type_ids_size;
    uint32_t type_ids_off;
    // ... muitos outros campos...
    uint32_t method_ids_size;
    uint32_t method_ids_off;
};

// Implementacao dummy para demonstrar o conceito

bool DexFileInspector::loadAndInspect(const std::string& dex_path, DexIR& out_ir) {
    out_ir.filename = dex_path;

    // 1. Abrir e Mapear o arquivo (Kernel I/O)
    size_t file_len;
    uint8_t* file_data = FileIO::mapFileReadOnly(dex_path, &file_len);

    if (file_data == nullptr || file_len < DEX_HEADER_SIZE) {
        Log::error(TAG, "Falha ao mapear ou arquivo muito pequeno: " + dex_path);
        return false;
    }
    
    // 2. Parsar as diferentes secoes
    if (!parseHeader(file_data, out_ir)) {
        Log::error(TAG, "Falha ao analisar o cabecalho DEX.");
        FileIO::unmapFile(file_data, file_len);
        return false;
    }

    // Usando as funcoes de parsing (simuladas)
    parseStrings(file_data, out_ir);
    parseTypes(file_data, out_ir);
    parseMethods(file_data, out_ir);

    FileIO::unmapFile(file_data, file_len);
    return true;
}

// Funcoes de parsing (simplificadas para o contexto)

bool DexFileInspector::parseHeader(const uint8_t* base, DexIR& ir) {
    const DexHeaderRaw* header = reinterpret_cast<const DexHeaderRaw*>(base);

    // Verificacao basica da Magia DEX
    if (std::memcmp(header->magic, "dex\n035\0", DEX_MAGIC_SIZE) != 0) {
        return false;
    }

    ir.checksum = header->checksum;
    ir.file_size = header->file_size;

    return true;
}

bool DexFileInspector::parseStrings(const uint8_t* base, DexIR& ir) {
    // Simulacao de parsing de 3 strings
    ir.strings.push_back({0, "Ljava/lang/Object;"});
    ir.strings.push_back({15, "myMethodName"});
    ir.strings.push_back({30, "Z"}); // Boolean
    return true;
}

bool DexFileInspector::parseTypes(const uint8_t* base, DexIR& ir) {
    // Simulacao: Type 0 = java/lang/Object, Type 1 = Z (Boolean)
    ir.types.push_back({0}); 
    ir.types.push_back({2}); 
    return true;
}

bool DexFileInspector::parseMethods(const uint8_t* base, DexIR& ir) {
    // Simulacao: Metodo 0 (class_idx=0 (Object), name_idx=1 (myMethodName), offset=0x1000)
    ir.methods.push_back({0, 1, 0x1000});
    return true;
}

/**
 * @brief Imprime um resumo detalhado da estrutura DexIR para o console do kernel.
 * * Uso Intenso de printf conforme solicitado.
 */
void DexFileInspector::printIRSummary(const DexIR& ir) const {
    printf("--- DEX IR Summary ---\n");
    printf("filename: %s\n", ir.filename.c_str());
    printf("filesize: %u bytes\n", ir.file_size);
    printf("checksum: 0x%x\n", ir.checksum);

    // Impressao do tamanho das tabelas (foco nos prints)
    printf("--------------------------\n");
    printf("dex strings : %zu\n", ir.strings.size());
    printf("dex types   : %zu\n", ir.types.size());
    printf("dex methods : %zu\n", ir.methods.size());
    printf("--------------------------\n");

    // Detalhamento das Strings
    printf("Strings Table (%zu entries):\n", ir.strings.size());
    for (size_t i = 0; i < ir.strings.size(); ++i) {
        printf("  [%zu] offset: %u, value: \"%s\"\n", 
               i, ir.strings[i].offset, ir.strings[i].value.c_str());
    }

    // Detalhamento dos Tipos
    printf("\nTypes Table (%zu entries):\n", ir.types.size());
    for (size_t i = 0; i < ir.types.size(); ++i) {
        // Tentativa de resolver a string (simulada)
        const std::string& type_str = (ir.types[i].string_idx < ir.strings.size()) ? 
                                       ir.strings[ir.types[i].string_idx].value : "N/A";
        printf("  [%zu] string_idx: %u, resolved: %s\n", 
               i, ir.types[i].string_idx, type_str.c_str());
    }

    // Detalhamento dos Metodos
    printf("\nMethods Table (%zu entries):\n", ir.methods.size());
    for (size_t i = 0; i < ir.methods.size(); ++i) {
        const DexMethod& method = ir.methods[i];
        
        // Tentativa de resolver o nome do método (simulada)
        const std::string& name_str = (method.name_idx < ir.strings.size()) ? 
                                       ir.strings[method.name_idx].value : "N/A";

        printf("  [%zu] class_idx: %u, name: %s, code_off: 0x%x\n",
               i, method.class_idx, name_str.c_str(), method.code_off);
    }
    printf("--------------------------\n");
}

} // namespace tools
} // namespace kernel
} // namespace comandro
