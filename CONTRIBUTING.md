# Contributing to Xcash Tech Core

Thank you for your interest in contributing to Xcash Tech Core! We welcome contributions from the community.

## 🌟 How to Contribute

### Reporting Bugs

If you find a bug, please open an issue on GitHub with:

- **Clear title and description**
- **Steps to reproduce** the issue
- **Expected behavior** vs actual behavior
- **Environment details** (OS, version, etc.)
- **Logs or screenshots** if applicable

### Suggesting Features

We love new ideas! To suggest a feature:

1. Check if it's already been suggested in [Issues](https://github.com/Xcash-Tech/xcash-tech-core/issues)
2. Open a new issue with the `enhancement` label
3. Describe the feature and its use case
4. Explain why it would benefit the project

### Code Contributions

#### Getting Started

1. **Fork the repository**
   ```bash
   # Click "Fork" on GitHub, then:
   git clone https://github.com/YOUR_USERNAME/xcash-tech-core.git
   cd xcash-tech-core
   ```

2. **Create a feature branch**
   ```bash
   git checkout -b feature/amazing-feature
   ```

3. **Make your changes**
   - Write clean, readable code
   - Follow existing code style
   - Add comments for complex logic
   - Update documentation if needed

4. **Test your changes**
   ```bash
   make release-test
   ```

5. **Commit your changes**
   ```bash
   git add .
   git commit -m "Add amazing feature"
   ```

6. **Push to your fork**
   ```bash
   git push origin feature/amazing-feature
   ```

7. **Open a Pull Request**
   - Go to the original repository on GitHub
   - Click "New Pull Request"
   - Select your branch
   - Fill out the PR template

#### Code Style Guidelines

- **C++ Code**:
  - Follow existing naming conventions
  - Use meaningful variable/function names
  - Keep functions focused and concise
  - Add comments for non-obvious code

- **Commits**:
  - Use clear, descriptive commit messages
  - Start with a verb (Add, Fix, Update, Remove)
  - Reference issue numbers when applicable
  - Example: `Fix: Resolve signature validation issue (#123)`

- **Pull Requests**:
  - Keep PRs focused on a single feature/fix
  - Provide clear description of changes
  - Link related issues
  - Respond to review feedback promptly

#### Branch Naming

- `feature/` - New features
- `fix/` - Bug fixes
- `docs/` - Documentation updates
- `refactor/` - Code refactoring
- `test/` - Test improvements

Examples:
- `feature/add-monitoring-endpoint`
- `fix/consensus-validation-error`
- `docs/update-build-instructions`

### Documentation Contributions

Documentation improvements are always welcome!

- Fix typos or unclear explanations
- Add examples or tutorials
- Improve API documentation
- Translate documentation (future)

To contribute to documentation:

1. Edit files in `docs/` directory
2. Follow Markdown best practices
3. Test links and code examples
4. Submit PR with clear description

### Testing

Before submitting a PR, ensure:

- [ ] Code compiles without errors
- [ ] All existing tests pass
- [ ] New features have tests (if applicable)
- [ ] Documentation is updated (if needed)

Run tests:
```bash
# Full test suite
make release-test

# Specific tests
./build/release/tests/unit_tests/unit_tests
```

## 🔍 Code Review Process

1. **Automated Checks**: CI/CD runs automated tests
2. **Maintainer Review**: Core team reviews code
3. **Feedback**: Address any requested changes
4. **Approval**: Once approved, PR will be merged
5. **Thanks**: Your contribution is appreciated! 🎉

## 📋 Pull Request Checklist

Before submitting, make sure:

- [ ] Code follows project style guidelines
- [ ] All tests pass
- [ ] Documentation is updated
- [ ] Commit messages are clear
- [ ] PR description explains changes
- [ ] Related issues are linked
- [ ] No merge conflicts

## 🐛 Reporting Security Issues

**DO NOT** open public issues for security vulnerabilities.

Instead, email: **security@xcash.tech**

Include:
- Description of the vulnerability
- Steps to reproduce
- Potential impact
- Suggested fix (if any)

## 💬 Community

Join our community for discussions and support:

- **Discord**: [discord.gg/4CAahnd](https://discord.gg/4CAahnd)
- **Twitter**: [@XcashTech](https://twitter.com/XcashTech)
- **Telegram**: [t.me/xcashtech](https://t.me/xcashtech)

## 📜 Code of Conduct

### Our Standards

We are committed to providing a welcoming and inclusive environment:

- **Be respectful** of differing viewpoints
- **Be patient** with newcomers
- **Be constructive** in feedback
- **Focus on** what is best for the community
- **Show empathy** towards others

### Unacceptable Behavior

- Harassment or discrimination
- Trolling or insulting comments
- Personal or political attacks
- Publishing others' private information
- Other conduct inappropriate for a professional setting

## 📄 License

By contributing, you agree that your contributions will be licensed under the [MIT License](LICENSE).

## 🙏 Recognition

Contributors are recognized in:
- Project README (major contributions)
- Release notes
- Community acknowledgments

Thank you for making Xcash Tech better! 🚀

---

**Questions?** Reach out on [Discord](https://discord.gg/4CAahnd) or open an issue.
