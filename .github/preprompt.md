# Noddle — Processus de Développement (Preprompt Orchestrateur)

> **Relire ce fichier au début de chaque session de développement.**
> Chemin : `.github/preprompt.md`

---

## 0. Initialisation de session

- [ ] Relire ce `preprompt.md`.
- [ ] Relire `knowledge.md` (racine du workspace).
- [ ] Consulter la todo list existante et la mettre à jour.
- [ ] Identifier la branche courante (`git branch --show-current`).

---

## 1. Architecture de la feature

- Déléguer à **Architect** la conception de l'interface/contrat.
- Produire : description fonctionnelle, types d'entrée/sortie, contrat de données, diagramme si nécessaire.
- Valider avec l'utilisateur avant de poursuivre si le périmètre n'est pas clair.

## 2. Écriture des tests (TDD)

- Déléguer à **Tester** la rédaction des tests **avant** l'implémentation.
- Suivre le skill TDD (`.github/skills/tdd/SKILL.md`) : plan de tests → tests Catch2/pytest → exécution → rapport.
- Les tests doivent échouer (phase Red).

## 3. Implémentation

- Déléguer au spécialiste approprié :
  - **Backend Dev** : moteur C++20, runtime, tenseur, CMake.
  - **OpenCV Expert** : nœuds OpenCV, conversion Mat/tensor.
  - **AI CV Expert** : inférence DNN/ONNX, pré/post-traitement.
  - **Frontend Dev** : UI Qt6/QtNodes, widgets, UX.
- Demander l'avis des agents spécialisés à chaque décision technique non triviale.
- Respecter les conventions : `cpp-conventions.instructions.md`, `qt-frontend-conventions.instructions.md`.

## 4. Exécution des tests

- Lancer `bash build.sh` (compile + ctest).
- Vérifier que **tous** les tests passent (anciens et nouveaux).
- Si des tests échouent → corriger le code et ré-exécuter jusqu'au vert complet.

## 5. Revue de code

- Déléguer à **Code Reviewer** : correctness, sécurité, performance, style, conventions.
- Appliquer les corrections demandées.
- Re-lancer les tests après corrections.

## 6. Commit sur la branche feature

- Déléguer à **Git Ops** :
  - Branche : `feature/<nom-de-la-feature>` (gitflow).
  - Messages : conventional commits (`feat:`, `fix:`, `test:`, `refactor:`, `docs:`).
  - Commits atomiques (un commit = un changement logique).

## 7. Itération

- Si la feature n'est pas complète ou si l'utilisateur demande des ajustements :
  - Revenir à l'étape appropriée (architecture, tests, implémentation).
  - Mettre à jour la todo list à chaque itération.

## 8. Merge sur develop

- Quand la feature est complète et validée :
  - Merge `feature/<nom>` → `develop`.
  - Déléguer à **Git Ops** pour le merge (fast-forward ou merge commit selon politique).
  - Supprimer la branche feature après merge.

## 9. Documentation

- Déléguer à **Documentalist** :
  - Relecture et mise à jour des commentaires de code (en anglais).
  - Mise à jour de la documentation API si applicable.
  - Mise à jour de `knowledge.md` avec les décisions prises.

## 10. Validation utilisateur

- Détailler à l'utilisateur la liste des features à tester manuellement.
- Fournir les étapes de test, les comportements attendus, et les cas limites.
- Attendre la validation explicite de l'utilisateur avant de poursuivre.

## 11. Merge sur master

- **Uniquement sur validation de l'utilisateur.**
- Déléguer à **Git Ops** : merge `develop` → `master`.

## 12. Release (sur demande)

- Sur demande explicite de l'utilisateur :
  - Créer la branche `release/vX.Y.Z` depuis `develop`.
  - Versionning sémantique :
    - **Majeur** (`X.0.0`) : changement d'architecture, breaking changes, refonte d'API.
    - **Mineur** (`0.X.0`) : nouvelles features, nouveaux nœuds, extensions d'API.
    - **Patch** (`0.0.X`) : corrections de bugs, polish UI, fixes de performance.
  - Tag git : `vX.Y.Z`.
  - Merge `release/vX.Y.Z` → `master` + `develop`.
  - Déléguer à **Git Ops** pour l'exécution.

---

## Règles transverses

| Règle | Détail |
|-------|--------|
| **knowledge.md** | Mettre à jour après chaque décision architecturale ou fait important. |
| **Todo list** | Maintenir à jour en permanence via `manage_todo_list`. |
| **Preprompt** | Relire `.github/preprompt.md` à chaque nouvelle tâche de développement. |
| **Avis spécialisé** | Toujours consulter l'agent expert avant une décision technique non triviale. |
| **Tests avant merge** | Aucun merge sans tous les tests au vert (build + ctest). |
| **Validation utilisateur** | Jamais de merge sur master sans validation explicite. |
| **Langue** | Code et commentaires en anglais, conversation en français. |
| **Confiance** | Ne pas deviner — demander clarification si le besoin est incertain. |

---

## Équipe d'agents disponibles

| Agent | Domaine |
|-------|---------|
| **Architect** | Conception système, contrats d'API, ADR |
| **Tester** | TDD, tests Catch2/pytest, couverture |
| **Backend Dev** | C++20 runtime, tenseur, CMake, performance |
| **Frontend Dev** | Qt6/QtNodes UI, widgets, UX |
| **OpenCV Expert** | Nœuds OpenCV, cv::, conversion formats |
| **AI CV Expert** | DNN/ONNX inference, modèles IA |
| **Code Reviewer** | Qualité, sécurité, style, conventions |
| **Git Ops** | Branches, commits, merges, tags, releases |
| **Documentalist** | Commentaires, API docs, knowledge.md |
