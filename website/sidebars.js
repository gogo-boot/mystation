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
    // Developer Guide Sidebar - For developers
    developerSidebar: [
        {
            type: 'category',
            label: '🏗️ Architecture',
            collapsed: false,
            items: [
                'developer-guide/index',
                'developer-guide/boot-process',
                'developer-guide/configuration-layers',
            ],
        },
        {
            type: 'category',
            label: '⚙️ Core Systems',
            collapsed: false,
            items: [
                'developer-guide/display-system',
                'developer-guide/button-system',
                'developer-guide/battery-management',
            ],
        },
        {
            type: 'category',
            label: '📡 APIs & Integration',
            collapsed: false,
            items: [
                'developer-guide/api-integration',
            ],
        },
        {
            type: 'category',
            label: '🛠️ Development',
            collapsed: false,
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
            collapsed: false,
            items: [
                'developer-guide/hardware-assembly',
                'hardware-setup',
            ],
        },
    ],

    // Maintainer Guide Sidebar
    maintainerSidebar: [
        {
            type: 'category',
            label: '📋 Maintainer Guide',
            collapsed: false,
            items: [
                'maintainer-guide/release-process',
                'maintainer-guide/ota-update',
                'maintainer-guide/github-actions',
                'maintainer-guide/certificate-bundle',
            ],
        },
    ],

    // Quick Reference Sidebar
    referenceSidebar: [
        {
            type: 'category',
            label: '📋 Quick Reference',
            collapsed: false,
            items: [
                'reference/configuration-keys-quick-reference',
            ],
        },
    ],
};

export default sidebars;
