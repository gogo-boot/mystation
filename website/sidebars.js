/**
 * Creating a sidebar enables you to:
 - create an ordered group of docs
 - render a sidebar for each doc of that group
 - provide next/previous navigation

 The sidebars can be generated from the filesystem, or explicitly defined here.

 Create as many sidebars as you want.
 */

// @ts-check

/** @type {import('@docusaurus/plugin-content-docs').SidebarsConfig} */
const sidebars = {
    // User Guide Sidebar - For end users
    userSidebar: [
        {
            type: 'category',
            label: '👋 Getting Started',
            items: [
                'user-guide/index',
                'user-guide/quick-start',
            ],
        },
        {
            type: 'category',
            label: '📱 Using MyStation',
            items: [
                'user-guide/understanding-display',
                'user-guide/button-controls',
            ],
        },
        {
            type: 'category',
            label: '🔧 Maintenance & Support',
            items: [
                'user-guide/troubleshooting',
            ],
        },
    ],

    // Developer Guide Sidebar - For developers
    developerSidebar: [
        {
            type: 'category',
            label: '🏗️ Architecture',
            items: [
                'developer-guide/index',
                'developer-guide/boot-process',
                'developer-guide/configuration-layers',
            ],
        },
        {
            type: 'category',
            label: '⚙️ Core Systems',
            items: [
                'developer-guide/display-system',
                'developer-guide/battery-management',
            ],
        },
        {
            type: 'category',
            label: '📡 APIs & Integration',
            items: [
                'developer-guide/api-integration',
            ],
        },
        {
            type: 'category',
            label: '🛠️ Development',
            items: [
                'developer-guide/development-setup',
                'developer-guide/testing',
                'developer-guide/testing-mocks',
                'developer-guide/testing-rtc',
                'developer-guide/run-book',
            ],
        },
        {
            type: 'category',
            label: '🔌 Hardware',
            items: [
                'developer-guide/hardware-assembly',
                'hardware-setup',
            ],
        },
    ],

    // Quick Reference Sidebar - For quick lookups
    maintainerSidebar: [
        {
            type: 'category',
            label: '📋 Maintainer Guide',
            items: [
                'maintainer-guide/ota-update',
                'maintainer-guide/github-actions',
                'maintainer-guide/certificate-bundle',
            ],
        },
    ],

    // Quick Reference Sidebar - For quick lookups
    referenceSidebar: [
        {
            type: 'category',
            label: '📋 Quick Reference',
            items: [
                'reference/configuration-keys-quick-reference',
            ],
        },
    ],
};

export default sidebars;

