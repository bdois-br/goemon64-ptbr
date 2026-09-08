Goemon 64 PT-BR - Beta 0.8.52 - FILE 05F + Barra de Energia SAFE

Base direta: Beta 0.8.51.

Correcoes desta build
---------------------
1. FILE 05F - retratos
   - Adiciona apenas os dialogos confirmados pelos prints recentes.
   - Recuo de 48 px a partir da 3a linha (line_index >= 2).
   - 1a e 2a linhas permanecem intactas.
   - Casos incluidos: Dancin' Comporte-se, Yae poderes secretos, Sasuke tem
     razao, Dancin' duas forcas, Dancin' Dos, Ebisumaru Eu nao sei e Goemon
     Canalhas.

2. FILE 05D - Barra de Energia
   - Corrige pontualmente o texto que ultrapassava a borda direita.
   - Novo layout:
       Automaticamente, sua Barra de
       Energia subiu um nivel!
   - A alteracao e feita dentro dos dois slots originais, sem deslocar offsets
     das mensagens seguintes.

Preservado
----------
- FILE 067: recuo de 48 px desde a 2a linha.
- FILE 069: recuo de 48 px desde a 3a linha.
- FILE 074: layout de retrato confirmado.
- Protecao anti-crash para caixas nativas de 9 rows / 72 px.
- Reflow global existente da base.
- Circunflexo refinado da 0.8.49.
- Quebra visual do Castelo dos Brinquedos Fantasmas.
- Cursor vermelho, fonte e demais acentos.

Observacao
----------
- No dialogo da Barra de Energia, o destaque amarelo original de "um" foi
  removido para manter a nova frase dentro do mesmo slot binario, sem alterar
  offsets ou estrutura do bloco.
