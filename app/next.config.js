/** @type {import('next').NextConfig} */
const nextConfig = {
	output: 'standalone',
	webpack: (config, { isServer }) => {
		if (isServer) {
			config.node = {
				...config.node,
				__dirname: true,
				__filename: true,
			};
		}
		return config;
	},
}

module.exports = nextConfig
