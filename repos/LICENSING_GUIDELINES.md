# ⚖️ Diretrizes de Licenciamento ComandroOS (GPLv3)

O Kernel e o Core do **ComandroOS** são distribuídos sob a **GNU General Public License, Versão 3 (GPLv3)**.

Este documento explica as obrigações e liberdades da GPLv3 para as distribuições que utilizam a base ComandroOS.

## 🔑 O Princípio do Copyleft (Reciprocidade)

A GPLv3 é uma licença *copyleft* forte. Isso significa:

1.  **Liberdade para Usar e Modificar:** Você pode usar, estudar, modificar e distribuir o ComandroOS livremente, incluindo o uso em produtos comerciais.
2.  **Obrigação de Compartilhar:** Se você **modificar** o kernel ComandroOS (ex: aplicar correções de bugs, adicionar otimizações de hardware, ou melhorar o desempenho) e distribuir essa versão modificada para terceiros, você **deve** disponibilizar o código-fonte dessa versão modificada sob os termos da GPLv3.

**Isso é fundamental:** Qualquer melhoria de código feita por uma distribuidora ou cliente deve retornar à comunidade, garantindo que o ComandroOS continue sendo o OS/kernel mais otimizado.

## 🖼️ Considerações de Interface Gráfica (DE)

* **Interfaces separadas:** Se a sua distribuição apenas **executa** um ambiente de desktop (DE) como GNOME ou KDE Plasma (que usam licenças compatíveis, como LGPL ou GPL), o código *dessas* interfaces não precisa ser coberto pela GPLv3, mas deve cumprir suas próprias licenças.
* **Módulos de Kernel:** Qualquer módulo que se ligue dinamicamente ao kernel ComandroOS deve ser compatível com a GPL. A Comandro não oferecerá suporte a *drivers* ou módulos de código proprietário que tentem contornar as proteções da GPL.

## 🔗 Links

* **[Texto Completo da Licença (GPLv3)]** - (Link para o arquivo LICENSE.md no repositório principal do kernel)
* **[Perguntas Frequentes (GNU)]** - (Link para o FAQ da GPL, se disponível)

---

**A Comandro LLC está comprometida com o software livre. Sua conformidade com a GPL é essencial para a saúde e a missão do ComandroOS.**
