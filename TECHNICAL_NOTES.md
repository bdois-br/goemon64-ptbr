# Notas técnicas do projeto

## Visão geral

A tradução exigiu alterações na renderização de diálogos do jogo, não apenas substituição de texto. O principal desafio foi acomodar frases maiores em português sem comprometer a estabilidade do sistema original.

## Fonte e acentos

O jogo original não oferece todos os caracteres necessários ao português brasileiro. A solução adotada foi compor visualmente letras acentuadas usando a fonte original e sinais gráficos sobrepostos.

O circunflexo passou por várias revisões até chegar à forma atual, usando um único glifo reduzido e reposicionado.

## Caixas de diálogo

Foram testadas várias larguras. O valor de 288 px apresentou o melhor equilíbrio entre espaço extra e preservação da interface.

Também foi implementada expansão vertical de 56 px para 72 px em situações seguras.

## Regra crítica das caixas nativas de 9 rows

Um dos crashes mais importantes do projeto foi isolado comparando builds antigas. A causa foi associada à manipulação horizontal de caixas que já nasciam nativamente com 9 rows / 72 px.

Regra adotada:

- caixas que nascem com 7 rows podem ser expandidas;
- caixas que nascem nativamente com 9 rows não devem ter sua textura horizontal reconstruída.

## Reflow automático

Foi desenvolvido um sistema de quebra automática que tenta mover palavras inteiras para a linha seguinte em vez de cortar palavras no meio.

Para evitar linhas extras causadas pela combinação entre quebra automática e newline já presente no script, foi criado um mecanismo de compensação de quebra visual (soft-newline debt).

## Retratos

Tentativas de detectar retratos dinamicamente por estado da caixa, textura ou posição de sprites produziram falsos positivos, falsos negativos ou maior risco de instabilidade.

A solução adotada passou a utilizar identificação por mensagens conhecidas do script, aplicando recuo horizontal apenas nos casos confirmados.

Exemplos de geometrias tratadas:

- FILE 067: recuo a partir da segunda linha;
- FILE 069: recuo a partir da terceira linha;
- outros grupos possuem regras específicas.

Uma melhoria futura recomendada é substituir assinaturas textuais por identificação direta por arquivo + offset/ID da mensagem.

## Casos especiais

### Castelo dos Brinquedos Fantasmas

Foi usada uma quebra exclusivamente visual com hífen, evitando alterar o estado interno da caixa.

### Cursor de menus

O cursor vermelho recebeu deslocamento específico para evitar sobreposição com opções traduzidas maiores.

### Barra de Energia

A mensagem foi ajustada pontualmente para caber nos slots originais sem deslocar offsets subsequentes.

## Builds experimentais descartadas

Entre as versões 0.8.40 e 0.8.46 foram testadas alterações diretas em um bloco sensível de diálogo. Essas versões apresentaram regressões, incluindo comportamento inesperado do jogo. O trabalho retornou à base estável 0.8.39 e seguiu uma linha mais conservadora.

## Melhorias futuras recomendadas

- identificar correções de retrato diretamente por ID/offset da mensagem;
- criar um simulador de largura de texto baseado nos glifos reais;
- criar um validador de códigos especiais do script;
- manter uma coleção de screenshots para testes de regressão;
- realizar uma revisão completa de acentos, espaçamento e textos fora da caixa.
