# Problemas conhecidos

Apesar de o projeto estar completo e publicável, a versão 0.8.52 ainda precisa de refinamentos visuais.

## Compatibilidade com outros mods

### Redub Mod

A versão atual do **Goemon 64 PT-BR v0.8.52** não é totalmente compatível com o `redub_mod.nrm`.

O Redub Mod substitui alguns scripts das cutscenes de abertura, final e outras cenas por versões próprias para restaurar as vozes japonesas. Esses scripts também contêm seus próprios textos em inglês, fazendo com que a tradução PT-BR seja substituída nessas cenas.

Está sendo estudada uma versão de compatibilidade que preserve as vozes japonesas do Redub Mod junto com os textos em português brasileiro.

## Acentuação

Alguns acentos são compostos visualmente a partir dos glifos da fonte original. Por isso, certos caracteres podem apresentar diferenças de tamanho, altura, centralização ou espaçamento.

## Texto fora da caixa

Ainda podem existir diálogos em que palavras ou linhas ultrapassem a área útil da caixa. O português tende a ocupar mais espaço que o texto original em inglês, e o jogo possui vários formatos diferentes de diálogo.

## Retratos

Algumas cenas posicionam retratos dentro da área da caixa. Vários casos foram tratados individualmente, mas ainda podem existir mensagens não identificadas nas quais o texto encoste ou passe por baixo do retrato.

## Quebras de linha e espaçamento

Podem existir situações com quebra de linha pouco natural, espaçamento irregular ou alinhamento visual que ainda necessite de revisão.

## Estabilidade

Durante o desenvolvimento foi identificado que manipular horizontalmente caixas que já nascem nativamente com 9 rows / 72 px pode causar crashes. A versão atual contém proteção específica contra esse comportamento.

## Como reportar

Ao encontrar um problema, informe:

- local/cena do jogo;
- personagem ou início da frase;
- captura de tela;
- comportamento esperado, quando possível.
