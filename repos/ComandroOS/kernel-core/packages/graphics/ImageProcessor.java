package comandro.kernel.core.packages.graphics;

import comandro.settings.debug.ComandroDebug;
import java.io.File;

/**
 * ImageProcessor: Subsistema de Processamento de Imagens e Gráficos.
 *
 * Fornece métodos otimizados para manipulação de dados de imagem diretamente
 * com o hardware gráfico (GPU) do sistema, minimizando o trabalho da CPU.
 * Todas as operações pesadas são delegadas a rotinas nativas/GPU.
 */
public final class ImageProcessor {

    // Identificador opaco (handle) para um recurso gráfico no lado nativo/GPU
    public static final class ImageHandle {
        final long nativePtr;
        public ImageHandle(long ptr) { this.nativePtr = ptr; }
    }

    // --- Métodos Nativos (Delegados à GPU/C++) ---
    private static native long native_load_image(String path);
    private static native long native_resize_optimized(long handle, int width, int height);
    private static native long native_apply_filter(long handle, int filterType);
    private static native void native_release_handle(long handle);

    // Tipos de Filtro (Exemplo)
    public static final int FILTER_GRAYSCALE = 1;
    public static final int FILTER_BLUR = 2;
    public static final int FILTER_EDGE_DETECT = 3;

    private ImageProcessor() {
        // Classe estática
    }

    /**
     * @brief Carrega uma imagem do sistema de arquivos e a carrega na memória gráfica.
     * @param imageFile O arquivo de imagem a ser carregado.
     * @return Um handle opaco para o recurso na memória nativa/GPU.
     */
    public static ImageHandle loadImage(File imageFile) {
        if (!imageFile.exists()) {
            ComandroDebug.e("IMG_PROC", "Arquivo não encontrado: " + imageFile.getPath());
            return null;
        }
        
        long handle = native_load_image(imageFile.getPath());
        
        if (handle > 0) {
            ComandroDebug.d("IMG_PROC", "Imagem carregada: " + imageFile.getName() + " (Handle: " + handle + ")");
            return new ImageHandle(handle);
        } else {
            ComandroDebug.e("IMG_PROC", "Falha ao carregar a imagem na memória gráfica.");
            return null;
        }
    }

    /**
     * @brief Redimensiona a imagem usando o hardware gráfico (GPU).
     * @param handle O handle da imagem original.
     * @param width A nova largura.
     * @param height A nova altura.
     * @return Um novo handle para a imagem redimensionada.
     */
    public static ImageHandle resize(ImageHandle handle, int width, int height) {
        if (handle == null || handle.nativePtr <= 0) {
            ComandroDebug.e("IMG_PROC", "Handle de imagem inválido para redimensionar.");
            return null;
        }
        long newHandle = native_resize_optimized(handle.nativePtr, width, height);
        if (newHandle > 0) {
            ComandroDebug.d("IMG_PROC", "Imagem redimensionada. Novo Handle: " + newHandle);
            // É responsabilidade da aplicação liberar o handle antigo, se necessário.
            return new ImageHandle(newHandle);
        } else {
            ComandroDebug.e("IMG_PROC", "Falha no redimensionamento otimizado.");
            return null;
        }
    }

    /**
     * @brief Aplica um filtro (grayscale, blur, etc.) na imagem usando a GPU.
     * @param handle O handle da imagem.
     * @param filterType O tipo de filtro a aplicar.
     * @return Um novo handle para a imagem processada.
     */
    public static ImageHandle applyFilter(ImageHandle handle, int filterType) {
        if (handle == null || handle.nativePtr <= 0) {
            ComandroDebug.e("IMG_PROC", "Handle de imagem inválido para filtrar.");
            return null;
        }
        
        long newHandle = native_apply_filter(handle.nativePtr, filterType);
        if (newHandle > 0) {
            ComandroDebug.d("IMG_PROC", "Filtro aplicado (" + filterType + "). Novo Handle: " + newHandle);
            return new ImageHandle(newHandle);
        } else {
            ComandroDebug.e("IMG_PROC", "Falha ao aplicar filtro.");
            return null;
        }
    }

    /**
     * @brief Libera a memória gráfica e o recurso nativo associado ao handle.
     * @param handle O handle do recurso a ser liberado.
     */
    public static void releaseImage(ImageHandle handle) {
        if (handle != null && handle.nativePtr > 0) {
            native_release_handle(handle.nativePtr);
            ComandroDebug.d("IMG_PROC", "Recurso gráfico liberado. Handle: " + handle.nativePtr);
            // O objeto Java Handle ainda existe, mas o recurso nativo foi liberado.
        }
    }
}
