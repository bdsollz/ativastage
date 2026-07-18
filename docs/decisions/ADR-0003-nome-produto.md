# ADR-0003 — Nome do produto: AtivaStage

- **Status:** Aceito
- **Data:** 2026-07-18
- **Contexto:** Decisão em aberto herdada (Plano Técnico Seção 15 / Relatório
  Seção 18): nome do produto e, por consequência, do serviço mDNS e bundle id.

## Decisão

O nome definitivo do produto é **AtivaStage** (confirmado pelo idealizador).

Consequências diretas:

- **Bundle id:** `br.com.ativa.ativastage` (já em uso no `AtivaStage.command` e
  nos helpers) — mantido.
- **Nome de exibição / janelas / instaladores:** "AtivaStage".
- **Marca nas telas de UI:** o placeholder "ProChurch AV" dos mockups do Stitch
  (`docs/design/`) deve ser substituído por "AtivaStage" quando a UI for
  construída (a partir da Fase 3).
- **Serviço mDNS:** nome definitivo a fechar na Fase 8 (Remote Server). Proposta:
  `_ativastage._tcp.local` (confirmar em ADR próprio ao iniciar `libremote`).

## Fecha

Remove do rol de "decisões em aberto" o item **nome do produto**. Permanecem
abertos: nome definitivo do serviço mDNS (depende só de confirmação), versões
bíblicas de domínio público a incluir, e demais itens da Seção 15 do plano.
