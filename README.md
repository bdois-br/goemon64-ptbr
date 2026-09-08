# Goemon 64 PT-BR

Tradução brasileira de **Mystical Ninja Starring Goemon** para o projeto **Goemon64Recompiled**.

**Versão atual:** Beta 0.8.52 — FILE 05F + Barra de Energia SAFE  
**Autor:** Bdois

## Sobre o projeto

O Goemon 64 PT-BR nasceu da vontade de um fã da série de jogar *Mystical Ninja Starring Goemon* em português brasileiro.

Este foi o **primeiro projeto amador do autor**, iniciado **sem experiência prévia em programação ou criação de mods**. O desenvolvimento contou com **amplo auxílio de Inteligência Artificial**, utilizada para análise, programação, investigação de erros, geração de builds e documentação e tradução dos diálogos. Os testes, decisões e validações dentro do jogo foram feitos manualmente pelo autor.

O trabalho foi além da tradução dos textos. Foi necessário adaptar partes do sistema de diálogos do jogo para lidar com textos maiores em português, acentuação, caixas de diálogo, retratos dos personagens, quebra automática de linhas e casos especiais de interface.

## Estado atual

O projeto está **completo, jogável e publicável**, mas ainda precisa de bastante polimento visual.

Os principais pontos conhecidos são:

- acentos que ainda podem precisar de ajustes de tamanho e posicionamento;
- textos que, em algumas situações, ainda podem ultrapassar ou invadir caixas de diálogo;
- diálogos com retratos que podem exigir correções específicas;
- pequenas inconsistências de espaçamento, quebra de linha e alinhamento;
- situações pouco comuns que podem não ter sido encontradas durante os testes.

A versão 0.8.52 deve ser entendida como uma tradução completa e funcional, mas **não como uma localização profissional totalmente revisada**.

Veja também: [Problemas conhecidos](KNOWN_ISSUES.md).

## Principais recursos

- Tradução PT-BR do jogo;
- suporte visual a caracteres acentuados;
- ajustes de largura e altura de caixas de diálogo;
- quebra automática de palavras em diálogos longos;
- proteção para caixas nativas de 9 linhas que causavam crashes em versões experimentais;
- correções específicas para textos próximos a retratos;
- correção do cursor vermelho em menus;
- ajustes pontuais em mensagens especiais, como o Castelo dos Brinquedos Fantasmas e a Barra de Energia.

## Screenshots
<img width="960" height="574" alt="Image" src="https://github.com/user-attachments/assets/cb0e313b-479b-4798-860b-fe9933e4c36e" />
<img width="960" height="578" alt="Image" src="https://github.com/user-attachments/assets/03095028-0f37-4473-aa84-92012346093d" />
<img width="960" height="573" alt="Image" src="https://github.com/user-attachments/assets/e9b946a4-a827-4dd6-929a-0d7970abd957" />
<img width="960" height="573" alt="Image" src="https://github.com/user-attachments/assets/daddb671-9f93-4a53-abb8-39ce1bc6410d" />
<img width="960" height="574" alt="Image" src="https://github.com/user-attachments/assets/0a47bbc1-250d-4e86-b9be-0bd6f4f597b8" />
<img width="960" height="573" alt="Image" src="https://github.com/user-attachments/assets/2dfe2efb-7344-4959-b6ee-d5b2db9f3f56" />
<img width="960" height="576" alt="Image" src="https://github.com/user-attachments/assets/8480ce4f-9c9b-4159-b2fb-d660624b696c" />
<img width="960" height="460" alt="Image" src="https://github.com/user-attachments/assets/3f7b930e-bee1-4b10-850e-aa34932c515b" />

## Instalação

1. Tenha uma instalação compatível do **Goemon64Recompiled**.
2. Baixe o arquivo `.nrm` da versão atual do mod.
3. Instale o mod pelo sistema de mods do Goemon64Recompiled.
4. Inicie o jogo com o mod habilitado.

> Este projeto **não distribui a ROM nem o jogo original**. É necessário possuir legalmente uma cópia compatível de *Mystical Ninja Starring Goemon*.

## Desenvolvimento

O código-fonte do mod está incluído neste repositório. A compilação utiliza o ecossistema de mods do Goemon64Recompiled/RecompModTool.

A build pública de referência é:

`Goemon64_PTBR_BETA0852_FILE05F_ENERGIA_SAFE.nrm`

Para detalhes sobre decisões técnicas, regressões e soluções adotadas, consulte [Notas técnicas](TECHNICAL_NOTES.md).

## Observação sobre Inteligência Artificial

A Inteligência Artificial foi uma ferramenta central neste projeto. Ela auxiliou na análise de código, criação e revisão de implementações, diagnóstico de crashes e regressões, organização da tradução e documentação técnica.

O projeto não foi gerado de forma totalmente automática: cada build foi testada dentro do jogo, e as decisões de manter, alterar ou descartar soluções foram tomadas pelo autor a partir dos resultados observados.

## Contribuições

Correções de acentuação, textos fora das caixas, quebras de linha e outros problemas visuais são bem-vindas. Ao reportar um problema, se possível envie uma captura de tela e informe em que parte do jogo ele ocorre.

## Aviso legal

Este é um projeto de fã, sem fins oficiais e sem afiliação com a Konami ou com os autores originais do jogo. Marcas, personagens, nomes e demais propriedades relacionadas a *Mystical Ninja Starring Goemon* pertencem aos seus respectivos detentores.
